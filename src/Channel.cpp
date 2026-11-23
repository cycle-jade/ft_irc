


#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string &name, Client *creator)
    : _name(name), _topic(""), _topicSetter(""), _key(""),
      _userLimit(0), _inviteOnly(false), _topicRestricted(false)
{
    // First member is automatically an operator
    _members[creator] = true;
}

Channel::~Channel() {}

// ─── Getters ──────────────────────────────────────────────────────────────

const std::string &Channel::getName()      const { return _name; }
const std::string &Channel::getTopic()     const { return _topic; }
const std::string &Channel::getKey()       const { return _key; }
int                Channel::getUserLimit() const { return _userLimit; }
int                Channel::getMemberCount() const { return (int)_members.size(); }
bool               Channel::isInviteOnly()       const { return _inviteOnly; }
bool               Channel::isTopicRestricted()  const { return _topicRestricted; }
bool               Channel::hasKey()             const { return !_key.empty(); }
bool               Channel::hasUserLimit()       const { return _userLimit > 0; }

const std::map<Client *, bool> &Channel::getMembers() const { return _members; }

// ─── Mode setters ─────────────────────────────────────────────────────────

void Channel::setInviteOnly(bool val)          { _inviteOnly = val; }
void Channel::setTopicRestricted(bool val)     { _topicRestricted = val; }
void Channel::setKey(const std::string &key)   { _key = key; }
void Channel::removeKey()                      { _key = ""; }
void Channel::setUserLimit(int limit)          { _userLimit = limit; }
void Channel::removeUserLimit()                { _userLimit = 0; }

void Channel::setTopic(const std::string &topic, Client *setter)
{
    _topic = topic;
    _topicSetter = setter ? setter->getNick() : "";
}

// ─── Member management ────────────────────────────────────────────────────

void Channel::addMember(Client *client)
{
    _members[client] = false; // joins as regular user
}

void Channel::removeMember(Client *client)
{
    _members.erase(client);
    // Also remove from invited list
    for (size_t i = 0; i < _invited.size(); ++i)
    {
        if (_invited[i] == client)
        {
            _invited.erase(_invited.begin() + i);
            break;
        }
    }
}

bool Channel::hasMember(Client *client) const
{
    return _members.find(client) != _members.end();
}

bool Channel::isOperator(Client *client) const
{
    std::map<Client *, bool>::const_iterator it = _members.find(client);
    return it != _members.end() && it->second == true;
}

void Channel::addOperator(Client *client)
{
    if (_members.count(client))
        _members[client] = true;
}

void Channel::removeOperator(Client *client)
{
    if (_members.count(client))
        _members[client] = false;
}

void Channel::addInvited(Client *client)
{
    if (!isInvited(client))
        _invited.push_back(client);
}

bool Channel::isInvited(Client *client) const
{
    for (size_t i = 0; i < _invited.size(); ++i)
        if (_invited[i] == client)
            return true;
    return false;
}

// ─── Broadcast ────────────────────────────────────────────────────────────

void Channel::broadcast(const std::string &msg, Client *except)
{
    for (std::map<Client *, bool>::iterator it = _members.begin();
         it != _members.end(); ++it)
    {
        if (it->first != except)
            it->first->sendMsg(msg);
    }
}