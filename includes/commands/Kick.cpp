

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"

// KICK <#channel> <nick> [:<reason>]
void cmdKick(Server &srv, Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS, client->getNick(),
                                  "KICK :Not enough parameters"));
        return;
    }

    const std::string &chanName = params[0];
    const std::string &targetNick = params[1];
    std::string reason = (params.size() > 2) ? params[2] : client->getNick();

    std::map<std::string, Channel *> &channels = srv.getChannels();
    std::map<std::string, Channel *>::iterator it = channels.find(chanName);
    if (it == channels.end())
    {
        client->sendMsg(makeReply(ERR_NOSUCHCHANNEL, client->getNick(),
                                  chanName + " :No such channel"));
        return;
    }
    Channel *channel = it->second;

    if (!channel->hasMember(client))
    {
        client->sendMsg(makeReply(ERR_NOTONCHANNEL, client->getNick(),
                                  chanName + " :You're not on that channel"));
        return;
    }
    if (!channel->isOperator(client))
    {
        client->sendMsg(makeReply(ERR_CHANOPRIVSNEEDED, client->getNick(),
                                  chanName + " :You're not channel operator"));
        return;
    }

    Client *target = srv.getClientByNick(targetNick);
    if (!target || !channel->hasMember(target))
    {
        client->sendMsg(makeReply(ERR_USERNOTINCHANNEL, client->getNick(),
                                  targetNick + " " + chanName + " :They aren't on that channel"));
        return;
    }

    // Broadcast KICK to everyone (including the kicked user) before removing
    std::string kickMsg = client->getPrefix() + " KICK " + chanName
                          + " " + targetNick + " :" + reason + "\r\n";
    channel->broadcast(kickMsg, NULL);

    channel->removeMember(target);

    if (channel->getMemberCount() == 0)
    {
        delete channel;
        channels.erase(it);
    }
}
