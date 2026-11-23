#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>

#define SERVER_NAME "ircserv"

// --- IRC Numeric Replies ---
// Registration
#define RPL_WELCOME         "001"
#define RPL_YOURHOST        "002"
#define RPL_CREATED         "003"
#define RPL_MYINFO          "004"

// Channel info
#define RPL_CHANNELMODEIS   "324"
#define RPL_NOTOPIC         "331"
#define RPL_TOPIC           "332"
#define RPL_NAMREPLY        "353"
#define RPL_ENDOFNAMES      "366"
#define RPL_INVITING        "341"

// Errors
#define ERR_NOSUCHNICK      "401"
#define ERR_NOSUCHCHANNEL   "403"
#define ERR_CANNOTSENDTOCHAN "404"
#define ERR_TOOMANYCHANNELS "405"
#define ERR_UNKNOWNCOMMAND  "421"
#define ERR_NONICKNAMEGIVEN "431"
#define ERR_ERRONEUSNICKNAME "432"
#define ERR_NICKNAMEINUSE   "433"
#define ERR_USERNOTINCHANNEL "441"
#define ERR_NOTONCHANNEL    "442"
#define ERR_USERONCHANNEL   "443"
#define ERR_NOTREGISTERED   "451"
#define ERR_NEEDMOREPARAMS  "461"
#define ERR_ALREADYREGISTRED "462"
#define ERR_PASSWDMISMATCH  "464"
#define ERR_CHANNELISFULL   "471"
#define ERR_UNKNOWNMODE     "472"
#define ERR_INVITEONLYCHAN  "473"
#define ERR_BADCHANNELKEY   "475"
#define ERR_CHANOPRIVSNEEDED "482"

// --- Helper Functions ---

// Split a string by a delimiter
inline std::vector<std::string> split(const std::string &str, char delim)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim))
        if (!token.empty())
            tokens.push_back(token);
    return tokens;
}

// Convert int to string (C++98 compatible)
inline std::string intToStr(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

// Trim \r from end of string (IRC lines end with \r\n)
inline std::string trimCR(const std::string &s)
{
    if (!s.empty() && s[s.size() - 1] == '\r')
        return s.substr(0, s.size() - 1);
    return s;
}

// Build a formatted IRC reply:
// :SERVER_NAME <numeric> <target> <message>\r\n
inline std::string makeReply(const std::string &numeric,
                              const std::string &target,
                              const std::string &msg)
{
    return ":" + std::string(SERVER_NAME) + " " + numeric + " " + target + " " + msg + "\r\n";
}

// Check if a channel name is valid (starts with # and no spaces)
inline bool isValidChannelName(const std::string &name)
{
    return name.size() > 1 && name[0] == '#'
           && name.find(' ') == std::string::npos
           && name.find(',') == std::string::npos
           && name.find(7) == std::string::npos; // no BELL char
}

// Check if a nickname is valid (alphanumeric + limited special chars)
inline bool isValidNick(const std::string &nick)
{
    if (nick.empty() || nick.size() > 9)
        return false;
    const std::string special = "-[]\\`^{}|_";
    for (size_t i = 0; i < nick.size(); ++i)
    {
        char c = nick[i];
        if (!std::isalnum(c) && special.find(c) == std::string::npos)
            return false;
    }
    return true;
}

// --- Welcome burst (001-004) ---
// Called once when NICK + USER + PASS are all satisfied.
// Defined here so both Nick.cpp and User.cpp share the same implementation.
#include "Client.hpp"

inline void sendWelcome(Client *client)
{
    const std::string nick = client->getNick();
    client->sendMsg(makeReply(RPL_WELCOME,  nick,
        ":Welcome to the IRC Network " + client->getPrefix()));
    client->sendMsg(makeReply(RPL_YOURHOST, nick,
        ":Your host is " SERVER_NAME ", running version 1.0"));
    client->sendMsg(makeReply(RPL_CREATED,  nick,
        ":This server was created just now"));
    client->sendMsg(makeReply(RPL_MYINFO,   nick,
        SERVER_NAME " 1.0 o itkol"));
}

#endif
