#include "Bot.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <csignal>

// ─── Signal handling ─────────────────────────────────────────────────────────
static bool g_running = true;
static void sigHandler(int) { g_running = false; }

// ─── Constructor / Destructor ─────────────────────────────────────────────────

Bot::Bot(const std::string &host, int port, const std::string &password)
    : _host(host), _port(port), _password(password), _fd(-1)
{
    _startTime = std::time(NULL);
    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGPIPE, SIG_IGN);
    std::srand(static_cast<unsigned int>(std::time(NULL)));
}

Bot::~Bot()
{
    if (_fd != -1)
        close(_fd);
}

// ─── Public: run ─────────────────────────────────────────────────────────────

void Bot::run()
{
    _connect();
    _register();
    _loop();
}

// ─── Setup ───────────────────────────────────────────────────────────────────

void Bot::_connect()
{
    // Resolve hostname (supports both IP and domain)
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    std::ostringstream portStr;
    portStr << _port;

    if (getaddrinfo(_host.c_str(), portStr.str().c_str(), &hints, &res) != 0)
        throw std::runtime_error("getaddrinfo() failed for host: " + _host);

    _fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (_fd < 0)
    {
        freeaddrinfo(res);
        throw std::runtime_error("socket() failed");
    }

    if (connect(_fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        freeaddrinfo(res);
        throw std::runtime_error("connect() failed — is the server running?");
    }
    freeaddrinfo(res);

    std::cout << "[Bot] Connected to " << _host << ":" << _port << std::endl;
}

void Bot::_register()
{
    // Standard IRC registration sequence
    _send("PASS " + _password);
    _send("NICK " + std::string(BOT_NICK));
    _send("USER " + std::string(BOT_USER) + " 0 * :" + std::string(BOT_REALNAME));
}

// ─── Event loop ──────────────────────────────────────────────────────────────

void Bot::_loop()
{
    pollfd pfd;
    pfd.fd     = _fd;
    pfd.events = POLLIN;

    while (g_running)
    {
        pfd.revents = 0;
        int ready = poll(&pfd, 1, 1000); // 1s timeout so SIGINT is responsive

        if (ready < 0)
        {
            if (!g_running) break;
            continue;
        }
        if (ready == 0)
            continue; // timeout, loop again

        if (pfd.revents & POLLIN)
            _receive();

        if (pfd.revents & (POLLHUP | POLLERR))
        {
            std::cout << "[Bot] Server disconnected." << std::endl;
            break;
        }
    }
    std::cout << "[Bot] Shutting down." << std::endl;
}

void Bot::_receive()
{
    char buf[RECV_BUFSIZE];
    std::memset(buf, 0, sizeof(buf));

    int bytes = recv(_fd, buf, sizeof(buf) - 1, 0);
    if (bytes <= 0)
    {
        g_running = false;
        return;
    }

    _recvBuf += std::string(buf, bytes);

    // Extract complete lines (\r\n terminated)
    size_t pos;
    while ((pos = _recvBuf.find("\r\n")) != std::string::npos)
    {
        std::string line = _recvBuf.substr(0, pos);
        _recvBuf.erase(0, pos + 2);
        if (!line.empty())
            _handleLine(line);
    }
}

// ─── IRC protocol ────────────────────────────────────────────────────────────

void Bot::_send(const std::string &msg)
{
    std::string full = msg + "\r\n";
    send(_fd, full.c_str(), full.size(), 0);
    std::cout << "[Bot] >> " << msg << std::endl;
}

void Bot::_sendPrivmsg(const std::string &target, const std::string &text)
{
    _send("PRIVMSG " + target + " :" + text);
}

void Bot::_handleLine(const std::string &line)
{
    std::cout << "[Bot] << " << line << std::endl;

    // PING :server  ->  PONG :server  (keep-alive)
    if (line.substr(0, 4) == "PING")
    {
        _handlePing(line);
        return;
    }

    // :server 001 ChanBot :Welcome...  -> join channel after welcome
    if (line.find(" 001 ") != std::string::npos)
    {
        std::cout << "[Bot] Registered. Joining " << BOT_CHANNEL << std::endl;
        _send("JOIN " + std::string(BOT_CHANNEL));
        return;
    }

    // :nick!user@host PRIVMSG #chan :text
    if (line.find(" PRIVMSG ") == std::string::npos)
        return;

    // Parse: prefix command target :text
    std::string prefix, target, text;
    size_t pos = 0;

    if (line[0] == ':')
    {
        size_t sp = line.find(' ');
        prefix = line.substr(1, sp - 1);
        pos = sp + 1;
    }

    // skip "PRIVMSG "
    size_t sp2 = line.find(' ', pos);
    if (sp2 == std::string::npos) return;
    pos = sp2 + 1;

    size_t sp3 = line.find(' ', pos);
    if (sp3 == std::string::npos) return;
    target = line.substr(pos, sp3 - pos);
    pos = sp3 + 1;

    if (pos < line.size() && line[pos] == ':')
        text = line.substr(pos + 1);
    else
        text = line.substr(pos);

    _handlePrivmsg(prefix, target, text);
}

void Bot::_handlePing(const std::string &line)
{
    // PING :token  ->  PONG :token
    size_t pos = line.find(':');
    if (pos != std::string::npos)
        _send("PONG :" + line.substr(pos + 1));
    else
        _send("PONG");
}

void Bot::_handlePrivmsg(const std::string &prefix,
                          const std::string &target,
                          const std::string &text)
{
    // Ignore messages from self
    std::string senderNick = _nickFromPrefix(prefix);
    if (senderNick == BOT_NICK)
        return;

    // Only react to messages starting with '!'
    if (text.empty() || text[0] != '!')
        return;

    // Reply target: if message was in a channel, reply there.
    // If it was a private message, reply to the sender.
    std::string replyTo = (target[0] == '#') ? target : senderNick;

    // Parse command and args
    std::string cmdpart = text.substr(1); // strip '!'
    size_t sp = cmdpart.find(' ');
    std::string cmd  = (sp == std::string::npos) ? cmdpart : cmdpart.substr(0, sp);
    std::string args = (sp == std::string::npos) ? ""      : _trim(cmdpart.substr(sp + 1));

    // Lowercase the command for case-insensitive matching
    for (size_t i = 0; i < cmd.size(); ++i)
        cmd[i] = std::tolower(cmd[i]);

    if      (cmd == "help")   _cmdHelp(replyTo);
    else if (cmd == "roll")   _cmdRoll(replyTo, args);
    else if (cmd == "flip")   _cmdFlip(replyTo);
    else if (cmd == "8ball")  _cmd8ball(replyTo, args);
    else if (cmd == "choose") _cmdChoose(replyTo, args);
    else if (cmd == "kick")   _cmdKick(target, args, senderNick);
    else if (cmd == "uptime") _cmdUptime(replyTo);
    else
        _sendPrivmsg(replyTo, "Unknown command. Type !help for a list.");
}

// ─── Command handlers ────────────────────────────────────────────────────────

void Bot::_cmdHelp(const std::string &target)
{
    _sendPrivmsg(target, "Available commands:");
    _sendPrivmsg(target, "  !roll <NdN>        — roll dice  (e.g. !roll 2d6)");
    _sendPrivmsg(target, "  !flip              — flip a coin");
    _sendPrivmsg(target, "  !8ball <question>  — ask the magic 8-ball");
    _sendPrivmsg(target, "  !choose a | b | c  — pick one option at random");
    _sendPrivmsg(target, "  !kick <nick> [msg] — kick someone (requires bot has op)");
    _sendPrivmsg(target, "  !uptime            — how long the bot has been running");
}

void Bot::_cmdRoll(const std::string &target, const std::string &args)
{
    // Parse NdN format — e.g. "2d6", "1d20", "3d4"
    std::string spec = args.empty() ? "1d6" : args;

    size_t dpos = spec.find('d');
    if (dpos == std::string::npos)
        dpos = spec.find('D');

    if (dpos == std::string::npos || dpos == 0 || dpos == spec.size() - 1)
    {
        _sendPrivmsg(target, "Usage: !roll NdN (e.g. !roll 2d6)");
        return;
    }

    int num   = std::atoi(spec.substr(0, dpos).c_str());
    int sides = std::atoi(spec.substr(dpos + 1).c_str());

    if (num <= 0 || num > 20 || sides <= 0 || sides > 100)
    {
        _sendPrivmsg(target, "Range: 1-20 dice, 2-100 sides.");
        return;
    }

    int total = 0;
    std::string rolls = "";
    for (int i = 0; i < num; ++i)
    {
        int r = (std::rand() % sides) + 1;
        total += r;
        if (!rolls.empty()) rolls += " + ";
        std::ostringstream oss;
        oss << r;
        rolls += oss.str();
    }

    std::ostringstream result;
    result << "Rolling " << spec << ": [" << rolls << "]";
    if (num > 1)
        result << " = " << total;
    _sendPrivmsg(target, result.str());
}

void Bot::_cmdFlip(const std::string &target)
{
    _sendPrivmsg(target, (std::rand() % 2 == 0) ? "Heads!" : "Tails!");
}

void Bot::_cmd8ball(const std::string &target, const std::string &question)
{
    static const char *answers[] = {
        "It is certain.",
        "It is decidedly so.",
        "Without a doubt.",
        "Yes, definitely.",
        "You may rely on it.",
        "As I see it, yes.",
        "Most likely.",
        "Outlook good.",
        "Yes.",
        "Signs point to yes.",
        "Reply hazy, try again.",
        "Ask again later.",
        "Better not tell you now.",
        "Cannot predict now.",
        "Concentrate and ask again.",
        "Don't count on it.",
        "My reply is no.",
        "My sources say no.",
        "Outlook not so good.",
        "Very doubtful."
    };
    static const int count = 20;

    if (question.empty())
    {
        _sendPrivmsg(target, "Ask a question! e.g. !8ball Will I pass the evaluation?");
        return;
    }

    _sendPrivmsg(target, std::string("\xF0\x9F\x8E\xB1 ") + answers[std::rand() % count]);
}

void Bot::_cmdChoose(const std::string &target, const std::string &args)
{
    if (args.empty())
    {
        _sendPrivmsg(target, "Usage: !choose option1 | option2 | option3");
        return;
    }

    std::vector<std::string> options = _split(args, '|');
    if (options.size() < 2)
    {
        _sendPrivmsg(target, "Give me at least 2 options separated by '|'");
        return;
    }

    // Trim whitespace from each option
    for (size_t i = 0; i < options.size(); ++i)
        options[i] = _trim(options[i]);

    std::string chosen = options[std::rand() % options.size()];
    _sendPrivmsg(target, "I choose: " + chosen);
}

void Bot::_cmdKick(const std::string &channel, const std::string &args,
                   const std::string &requesterNick)
{
    // Only works if the message was sent in a channel
    if (channel.empty() || channel[0] != '#')
    {
        _sendPrivmsg(requesterNick, "!kick only works in a channel.");
        return;
    }

    if (args.empty())
    {
        _sendPrivmsg(channel, "Usage: !kick <nick> [reason]");
        return;
    }

    size_t sp = args.find(' ');
    std::string targetNick = (sp == std::string::npos) ? args : args.substr(0, sp);
    std::string reason     = (sp == std::string::npos) ? "Kicked by " + requesterNick
                                                        : args.substr(sp + 1);

    // Bot must have op in the channel for this to work
    _send("KICK " + channel + " " + targetNick + " :" + reason);
}

void Bot::_cmdUptime(const std::string &target)
{
    time_t now     = std::time(NULL);
    long   elapsed = static_cast<long>(now - _startTime);

    long hours   = elapsed / 3600;
    long minutes = (elapsed % 3600) / 60;
    long seconds = elapsed % 60;

    std::ostringstream oss;
    oss << "Uptime: "
        << hours   << "h "
        << minutes << "m "
        << seconds << "s";
    _sendPrivmsg(target, oss.str());
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

std::string Bot::_nickFromPrefix(const std::string &prefix) const
{
    // prefix format: nick!user@host
    size_t pos = prefix.find('!');
    return (pos == std::string::npos) ? prefix : prefix.substr(0, pos);
}

std::vector<std::string> Bot::_split(const std::string &s, char delim) const
{
    std::vector<std::string> tokens;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim))
        tokens.push_back(token);
    return tokens;
}

std::string Bot::_trim(const std::string &s) const
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}