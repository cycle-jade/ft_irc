

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"

// INVITE <nick> <#channel>
void cmdInvite(Server &srv, Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS, client->getNick(),
                                  "INVITE :Not enough parameters"));
        return;
    }

    const std::string &targetNick = params[0];
    const std::string &chanName   = params[1];

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
    if (channel->isInviteOnly() && !channel->isOperator(client))
    {
        client->sendMsg(makeReply(ERR_CHANOPRIVSNEEDED, client->getNick(),
                                  chanName + " :You're not channel operator"));
        return;
    }

    Client *target = srv.getClientByNick(targetNick);
    if (!target)
    {
        client->sendMsg(makeReply(ERR_NOSUCHNICK, client->getNick(),
                                  targetNick + " :No such nick"));
        return;
    }
    if (channel->hasMember(target))
    {
        client->sendMsg(makeReply(ERR_USERONCHANNEL, client->getNick(),
                                  targetNick + " " + chanName + " :is already on channel"));
        return;
    }

    channel->addInvited(target);

    // Confirm to the inviter
    client->sendMsg(makeReply(RPL_INVITING, client->getNick(),
                               targetNick + " " + chanName));

    // Notify the invited user
    target->sendMsg(client->getPrefix() + " INVITE " + targetNick
                    + " :" + chanName + "\r\n");
}
