


#include "Server.hpp"
#include "utils.hpp"
#include "commands.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <cerrno>

// ─── Static signal flag ───────────────────────────────────────────────────
static bool g_running = true;

static void signalHandler(int /*sig*/)
{
    g_running = false;
}

// ─── Constructor / Destructor ─────────────────────────────────────────────

Server::Server(int port, const std::string &password)
    : _port(port), _password(password), _serverFd(-1)
{
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN); // Ignore broken pipe -- handle via send() return value

    _initSocket();
    _bindAndListen();

    std::cout << "Server listening on port " << _port << std::endl;
}

Server::~Server()
{
    for (std::map<int, Client *>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    for (std::map<std::string, Channel *>::iterator it = _channels.begin();
         it != _channels.end(); ++it)
        delete it->second;

    if (_serverFd != -1)
        close(_serverFd);
}

// ─── Setup ────────────────────────────────────────────────────────────────

void Server::_initSocket()
{
    // AF_INET = IPv4, SOCK_STREAM = TCP
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw std::runtime_error("socket() failed");

    // SO_REUSEADDR: restart server immediately after crash without waiting
    // for the OS to release the port (avoids "Address already in use")
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt() failed");

    // O_NONBLOCK: all I/O on this fd is non-blocking
    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl() failed");
}

void Server::_bindAndListen()
{
    // sockaddr_in: tells the OS which address/port to claim
    // htons() converts port to network byte order (big-endian)
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY; // listen on all network interfaces

    if (bind(_serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed - port may be in use");

    // listen(): make socket passive -- ready to accept() incoming connections
    // SOMAXCONN is the system max for pending connection queue
    if (listen(_serverFd, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");

    // Register server fd in poll list
    // POLLIN = notify me when accept() won't block (new connection arrived)
    pollfd serverPfd;
    serverPfd.fd      = _serverFd;
    serverPfd.events  = POLLIN;
    serverPfd.revents = 0;
    _pollfds.push_back(serverPfd);
}

// ─── Main event loop ──────────────────────────────────────────────────────

void Server::run()
{
    while (g_running)
    {
        // poll() blocks until at least one fd is ready, or a signal arrives.
        // -1 timeout = wait indefinitely (no CPU spinning)
        int ready = poll(_pollfds.data(), _pollfds.size(), -1);

        if (ready < 0)
        {
            if (!g_running) break; // SIGINT/SIGTERM -- clean exit
            if (errno == EINTR)    // other signal interrupted -- retry
                continue;
            throw std::runtime_error("poll() failed");
        }

        // Snapshot size: _pollfds can change during the loop
        // (accept adds entries, disconnect removes them)
        size_t size = _pollfds.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (!(_pollfds[i].revents & POLLIN))
                continue;

            if (_pollfds[i].fd == _serverFd)
                _acceptNewClient();
            else
                _handleClientData(_pollfds[i].fd);
        }
    }
    std::cout << "\nServer shutting down." << std::endl;
}

// ─── Connection handling ──────────────────────────────────────────────────

void Server::_acceptNewClient()
{
    struct sockaddr_in clientAddr;
    socklen_t          addrLen = sizeof(clientAddr);

    // accept() creates a NEW fd for the specific client connection.
    // The server fd keeps listening for more connections.
    int clientFd = accept(_serverFd, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientFd < 0)
        return; // EAGAIN/EWOULDBLOCK -- no connection actually pending (race), skip

    // Make client fd non-blocking too
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
    {
        close(clientFd);
        return;
    }

    // Convert client IP to human-readable string
    std::string hostaddr = inet_ntoa(clientAddr.sin_addr);

    // Create Client object
    Client *client = new Client(clientFd, hostaddr);
    _clients[clientFd] = client;

    // Register client fd in poll list
    pollfd clientPfd;
    clientPfd.fd      = clientFd;
    clientPfd.events  = POLLIN;
    clientPfd.revents = 0;
    _pollfds.push_back(clientPfd);

    std::cout << "New connection from " << hostaddr
              << " (fd=" << clientFd << ")" << std::endl;
}

void Server::_handleClientData(int fd)
{
    char buf[512]; // IRC max message length is 512 bytes
    std::memset(buf, 0, sizeof(buf));

    int bytes = recv(fd, buf, sizeof(buf) - 1, 0);

    if (bytes <= 0)
    {
        // bytes == 0: client closed connection cleanly
        // bytes <  0: error (ECONNRESET, etc.)
        disconnectClient(fd);
        return;
    }

    // Append raw data to this client's receive buffer
    // (could be partial -- TCP does not guarantee full messages)
    Client *client = _clients[fd];
    client->appendToBuffer(std::string(buf, bytes));

    // Extract and process every complete message (\r\n terminated) in the buffer
    while (client->hasCompleteMessage())
    {
        std::string msg = client->extractMessage();
        if (!msg.empty())
            _processMessage(fd, msg);
    }
}

void Server::disconnectClient(int fd)
{
    std::map<int, Client *>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Client *client = it->second;
    std::cout << "Client disconnected: " << client->getNick()
              << " (fd=" << fd << ")" << std::endl;

    // Remove from every channel they were in
    for (std::map<std::string, Channel *>::iterator ch = _channels.begin();
         ch != _channels.end(); )
    {
        ch->second->removeMember(client);

        // If channel is now empty, delete it
        if (ch->second->getMemberCount() == 0)
        {
            delete ch->second;
            _channels.erase(ch++); // C++98: erase() returns void, use post-increment
        }
        else
            ++ch;
    }

    _removePollFd(fd);
    close(fd);
    delete client;
    _clients.erase(it);
}

// ─── Message processing ───────────────────────────────────────────────────

void Server::_processMessage(int fd, const std::string &rawMsg)
{
    Client *client = _clients[fd];

    // IRC message format: [':' prefix SPACE] COMMAND [params] [':' trailing]
    // Examples:
    //   PASS secretpassword
    //   NICK alice
    //   PRIVMSG #general :Hello everyone!

    std::string msg = trimCR(rawMsg);
    if (msg.empty())
        return;

    // Skip optional prefix (':server.name ...' -- only from server-to-server,
    // but some clients send it anyway)
    size_t pos = 0;
    if (msg[0] == ':')
    {
        pos = msg.find(' ');
        if (pos == std::string::npos) return;
        pos++; // skip past the space
    }

    // Extract command (always uppercase in IRC)
    size_t cmdEnd = msg.find(' ', pos);
    std::string cmd = msg.substr(pos, cmdEnd - pos);
    for (size_t i = 0; i < cmd.size(); ++i)
        cmd[i] = std::toupper(cmd[i]);

    // Extract parameters
    std::vector<std::string> params;
    if (cmdEnd != std::string::npos)
    {
        std::string rest = msg.substr(cmdEnd + 1);
        // A parameter starting with ':' is the "trailing" param -- it can contain spaces
        while (!rest.empty())
        {
            if (rest[0] == ':')
            {
                params.push_back(rest.substr(1)); // everything after ':' is one param
                break;
            }
            size_t space = rest.find(' ');
            params.push_back(rest.substr(0, space));
            if (space == std::string::npos) break;
            rest = rest.substr(space + 1);
        }
    }

    _dispatchCommand(client, cmd, params);
}

void Server::_dispatchCommand(Client *client, const std::string &cmd,
                               const std::vector<std::string> &params)
{

    if (cmd == "CAP") { return; } // ignors CAP (capabiliti list)

    // PING must be answered before and after registration
    if (cmd == "PING")
    {
        std::string token = params.empty() ? "ircserv" : params[0];
        client->sendMsg("PONG ircserv :" + token + "\r\n");
        return;
    }

    // Commands allowed before registration
    if (cmd == "PASS")   { cmdPass(*this, client, params);    return; }
    if (cmd == "NICK")   { cmdNick(*this, client, params);    return; }
    if (cmd == "USER")   { cmdUser(*this, client, params);    return; }
    if (cmd == "QUIT")   { cmdQuit(*this, client, params);    return; }

    // All other commands require full registration
    if (!client->isAuthenticated())
    {
        client->sendMsg(makeReply(ERR_NOTREGISTERED, client->getNick(),
                                  ":You have not registered"));
        return;
    }

    if (cmd == "JOIN")    { cmdJoin(*this, client, params);    return; }
    if (cmd == "PART")    { cmdPart(*this, client, params);    return; }
    if (cmd == "PRIVMSG") { cmdPrivmsg(*this, client, params); return; }
    if (cmd == "NOTICE")  { cmdPrivmsg(*this, client, params); return; }
    if (cmd == "KICK")    { cmdKick(*this, client, params);    return; }
    if (cmd == "INVITE")  { cmdInvite(*this, client, params);  return; }
    if (cmd == "TOPIC")   { cmdTopic(*this, client, params);   return; }
    if (cmd == "MODE")    { cmdMode(*this, client, params);    return; }

    // Unknown command
    client->sendMsg(makeReply(ERR_UNKNOWNCOMMAND, client->getNick(),
                               cmd + " :Unknown command"));
}

// ─── Helpers ─────────────────────────────────────────────────────────────

Client *Server::getClientByNick(const std::string &nick)
{
    for (std::map<int, Client *>::iterator it = _clients.begin();
         it != _clients.end(); ++it)
    {
        if (it->second->getNick() == nick)
            return it->second;
    }
    return NULL;
}

void Server::_removePollFd(int fd)
{
    for (size_t i = 0; i < _pollfds.size(); ++i)
    {
        if (_pollfds[i].fd == fd)
        {
            _pollfds.erase(_pollfds.begin() + i);
            return;
        }
    }
}

// ─── Public: getOrCreateChannel ──────────────────────────────────────────

Channel *Server::getOrCreateChannel(const std::string &name, Client *creator)
{
    std::map<std::string, Channel *>::iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    Channel *ch = new Channel(name, creator);
    _channels[name] = ch;
    return ch;
}
