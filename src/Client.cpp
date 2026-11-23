#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>

Client::Client(int fd, const std::string &hostaddr)
    : _fd(fd), _nick("*"), _username(""), _realname(""),
      _hostaddr(hostaddr), _passwd(false), _registered(false)
{}

Client::~Client() {}

// --- Getters ---

int                Client::getFd()       const { return _fd; }
const std::string &Client::getNick()     const { return _nick; }
const std::string &Client::getUsername() const { return _username; }
const std::string &Client::getRealname() const { return _realname; }
const std::string &Client::getHostaddr() const { return _hostaddr; }
bool               Client::isAuthenticated() const { return _registered; }
bool               Client::hasPasswd()   const { return _passwd; }
bool               Client::hasNick()     const { return _nick != "*" && !_nick.empty(); }
bool               Client::hasUser()     const { return !_username.empty(); }

// --- Setters ---

void Client::setNick(const std::string &nick)         { _nick = nick; }
void Client::setUsername(const std::string &username) { _username = username; }
void Client::setRealname(const std::string &realname) { _realname = realname; }
void Client::setPasswd(bool val)                      { _passwd = val; }
void Client::setRegistered()                          { _registered = true; }

// --- Buffer ---

void Client::appendToBuffer(const std::string &data)
{
    _recvBuffer += data;
}

// IRC messages are terminated by \r\n
bool Client::hasCompleteMessage() const
{
    return _recvBuffer.find("\r\n") != std::string::npos;
}

// Extracts one complete message (without the \r\n) from the buffer
std::string Client::extractMessage()
{
    size_t pos = _recvBuffer.find("\r\n");
    if (pos == std::string::npos)
        return "";
    std::string msg = _recvBuffer.substr(0, pos);
    _recvBuffer.erase(0, pos + 2);
    return msg;
}

// --- Send ---

void Client::sendMsg(const std::string &msg)
{
    // send() can fail if client disconnected; caller handles cleanup
    if (send(_fd, msg.c_str(), msg.size(), 0) < 0)
        std::cerr << "send() failed for fd " << _fd << std::endl;
}

// --- Prefix ---

std::string Client::getPrefix() const
{
    return ":" + _nick + "!" + _username + "@" + _hostaddr;
}