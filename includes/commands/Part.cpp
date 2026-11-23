#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"

// PART <#channel>[,<#channel2>] [:<reason>]
void cmdPart(Server &srv, Client *client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS, client->getNick(),
                                  "PART :Not enough parameters"));
        return;
    }

    std::string reason = (params.size() > 1) ? params[1] : client->getNick();
    std::vector<std::string> chans = split(params[0], ',');

    for (size_t i = 0; i < chans.size(); ++i)
    {
        const std::string &chanName = chans[i];
        std::map<std::string, Channel *> &channels = srv.getChannels();
        std::map<std::string, Channel *>::iterator it = channels.find(chanName);

        if (it == channels.end())
        {
            client->sendMsg(makeReply(ERR_NOSUCHCHANNEL, client->getNick(),
                                      chanName + " :No such channel"));
            continue;
        }
        Channel *channel = it->second;
        if (!channel->hasMember(client))
        {
            client->sendMsg(makeReply(ERR_NOTONCHANNEL, client->getNick(),
                                      chanName + " :You're not on that channel"));
            continue;
        }

        // Broadcast PART before removing (so sender sees it too)
        std::string partMsg = client->getPrefix() + " PART " + chanName
                              + " :" + reason + "\r\n";
        channel->broadcast(partMsg, NULL);

        channel->removeMember(client);

        // Clean up empty channels
        if (channel->getMemberCount() == 0)
        {
            delete channel;
            channels.erase(it);
        }
    }
}
