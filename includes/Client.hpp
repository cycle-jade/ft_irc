#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <netinet/in.h>

class Client
{
public:
    Client(int fd, const std::string &hostaddr);
    ~Client();

    // --- Getters ---
    int                 getFd()       const;
    const std::string  &getNick()     const;
    const std::string  &getUsername() const;
    const std::string  &getRealname() const;
    const std::string  &getHostaddr() const;
    bool                isAuthenticated() const;
    bool                hasPasswd()   const;
    bool                hasNick()     const;
    bool                hasUser()     const;

    // --- Setters ---
    void setNick(const std::string &nick);
    void setUsername(const std::string &username);
    void setRealname(const std::string &realname);
    void setPasswd(bool val);
    void setRegistered();

    // --- Buffer (partial message handling) ---
    void               appendToBuffer(const std::string &data);
    bool               hasCompleteMessage() const;
    std::string        extractMessage();

    // --- Send ---
    void               sendMsg(const std::string &msg);

    // --- Prefix (used in IRC replies) ---
    // Format: :nick!username@host
    std::string        getPrefix() const;

private:
    int          _fd;
    std::string  _nick;
    std::string  _username;
    std::string  _realname;
    std::string  _hostaddr;

    bool         _passwd;       // PASS received and correct
    bool         _registered;   // NICK + USER done (fully logged in)

    std::string  _recvBuffer;   // accumulates partial TCP data

    // Non-copyable
    Client(const Client &);
    Client &operator=(const Client &);
};

#endif