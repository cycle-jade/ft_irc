


#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include <vector>

class Client;

class Channel
{
public:
    Channel(const std::string &name, Client *creator);
    ~Channel();

    // --- Getters ---
    const std::string  &getName()     const;
    const std::string  &getTopic()    const;
    const std::string  &getKey()      const;
    int                 getUserLimit() const;

    // --- Mode flags ---
    bool isInviteOnly()   const;
    bool isTopicRestricted() const;
    bool hasKey()         const;
    bool hasUserLimit()   const;

    // --- Mode setters ---
    void setInviteOnly(bool val);
    void setTopicRestricted(bool val);
    void setKey(const std::string &key);
    void removeKey();
    void setUserLimit(int limit);
    void removeUserLimit();
    void setTopic(const std::string &topic, Client *setter);

    // --- Member management ---
    void    addMember(Client *client);
    void    removeMember(Client *client);
    bool    hasMember(Client *client)   const;
    bool    isOperator(Client *client)  const;
    void    addOperator(Client *client);
    void    removeOperator(Client *client);
    void    addInvited(Client *client);
    bool    isInvited(Client *client)   const;

    const std::map<Client *, bool> &getMembers() const; // value = is_operator

    // --- Broadcast ---
    void broadcast(const std::string &msg, Client *except = NULL);

    int  getMemberCount() const;

private:
    std::string              _name;
    std::string              _topic;
    std::string              _topicSetter;
    std::string              _key;
    int                      _userLimit;

    // Mode flags
    bool                     _inviteOnly;
    bool                     _topicRestricted;

    // Members: Client* -> true if operator
    std::map<Client *, bool> _members;

    // Invited list (for invite-only mode)
    std::vector<Client *>    _invited;

    // Non-copyable
    Channel(const Channel &);
    Channel &operator=(const Channel &);
};

#endif