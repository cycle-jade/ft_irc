


#ifndef SERVER_HPP
#define SERVER_HPP
 
#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <netinet/in.h>
 
#include "Client.hpp"
#include "Channel.hpp"
 
#include "utils.hpp"

#define MAX_CLIENTS 1024

class Client;
class Channel;
 
class Server
{
public:
    Server(int port, const std::string &password);
    ~Server();
 
    void run();

    // --- Public accessors (used by command handlers) ---
    const std::string                &getPassword() const { return _password; }
    std::map<std::string, Channel *> &getChannels()       { return _channels; }
    Channel *getOrCreateChannel(const std::string &name, Client *creator);
    Client  *getClientByNick(const std::string &nick);
    void     disconnectClient(int fd);

private:
    // --- Config ---
    int                 _port;
    std::string         _password;
    int                 _serverFd;
 
    // --- Poll ---
    std::vector<pollfd> _pollfds;
 
    // --- Data ---
    std::map<int, Client *>         _clients;   // fd -> Client
    std::map<std::string, Channel *> _channels;  // name -> Channel
 
    // --- Setup ---
    void _initSocket();
    void _bindAndListen();
 
    // --- Event loop ---
    void _acceptNewClient();
    void _handleClientData(int fd);

    // --- Message processing ---
    void _processMessage(int fd, const std::string &message);
    void _dispatchCommand(Client *client, const std::string &cmd,
                          const std::vector<std::string> &params);
 
    // --- Helpers ---
    void _removePollFd(int fd);
 
    // Non-copyable
    Server(const Server &);
    Server &operator=(const Server &);
};
 
#endif