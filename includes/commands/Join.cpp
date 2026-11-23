
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"

// Helper: send the full channel name list (353 + 366) to a client
static void sendNames(Client *client, Channel *channel)
{
    // Build member list: "@nick" for ops, "nick" for regular
    std::string names;
    const std::map<Client *, bool> &members = channel->getMembers();
    for (std::map<Client *, bool>::const_iterator it = members.begin();
         it != members.end(); ++it)
    {
        if (!names.empty()) names += " ";
        if (it->second) names += "@"; // operator prefix
        names += it->first->getNick();
    }
    // 353: "= #channel :@op nick1 nick2"  ('=' = public channel)
    client->sendMsg(makeReply(RPL_NAMREPLY, client->getNick(),
                               "= " + channel->getName() + " :" + names));
    client->sendMsg(makeReply(RPL_ENDOFNAMES, client->getNick(),
                               channel->getName() + " :End of /NAMES list"));
}

// JOIN <#channel>[,<#channel2>] [key[,key2]]
void cmdJoin(Server &srv, Client *client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS, client->getNick(),
                                  "JOIN :Not enough parameters"));
        return;
    }

    // IRC allows joining multiple channels: JOIN #a,#b key1,key2
    std::vector<std::string> chans  = split(params[0], ',');
    std::vector<std::string> keys   = (params.size() > 1) ? split(params[1], ',') : std::vector<std::string>();

    for (size_t i = 0; i < chans.size(); ++i)
    {
        const std::string &chanName = chans[i];
        std::string key = (i < keys.size()) ? keys[i] : "";

        if (!isValidChannelName(chanName))
        {
            client->sendMsg(makeReply(ERR_NOSUCHCHANNEL, client->getNick(),
                                      chanName + " :No such channel"));
            continue;
        }

        // Get existing channel or create new one (client becomes first member/op)
        bool isNew = (srv.getChannels().find(chanName) == srv.getChannels().end());
        Channel *channel = srv.getOrCreateChannel(chanName, client);

        // Already in channel? Skip
        if (!isNew && channel->hasMember(client))
            continue;

        // --- Mode checks (only on existing channels) ---
        if (!isNew)
        {
            if (channel->isInviteOnly() && !channel->isInvited(client))
            {
                client->sendMsg(makeReply(ERR_INVITEONLYCHAN, client->getNick(),
                                          chanName + " :Cannot join channel (+i)"));
                continue;
            }
            if (channel->hasKey() && channel->getKey() != key)
            {
                client->sendMsg(makeReply(ERR_BADCHANNELKEY, client->getNick(),
                                          chanName + " :Cannot join channel (+k)"));
                continue;
            }
            if (channel->hasUserLimit() &&
                channel->getMemberCount() >= channel->getUserLimit())
            {
                client->sendMsg(makeReply(ERR_CHANNELISFULL, client->getNick(),
                                          chanName + " :Cannot join channel (+l)"));
                continue;
            }

            channel->addMember(client);
        }
        // If isNew: creator was already added in Channel constructor

        // Broadcast JOIN to everyone in the channel (including the joiner)
        std::string joinMsg = client->getPrefix() + " JOIN " + chanName + "\r\n";
        channel->broadcast(joinMsg, NULL);

        // Send topic (or RPL_NOTOPIC) to the new member
        if (channel->getTopic().empty())
            client->sendMsg(makeReply(RPL_NOTOPIC, client->getNick(),
                                      chanName + " :No topic is set"));
        else
            client->sendMsg(makeReply(RPL_TOPIC, client->getNick(),
                                      chanName + " :" + channel->getTopic()));

        // Send NAMES list
        sendNames(client, channel);
    }
}
