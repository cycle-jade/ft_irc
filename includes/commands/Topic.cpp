#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"

// TOPIC <#channel> [:<new topic>]
// With no new topic: returns current topic. With topic: sets it (if allowed).
void cmdTopic(Server &srv, Client *client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS, client->getNick(),
                                  "TOPIC :Not enough parameters"));
        return;
    }

    const std::string &chanName = params[0];
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

    // Query topic
    if (params.size() == 1)
    {
        const std::string &topic = channel->getTopic();
        if (topic.empty())
            client->sendMsg(makeReply(RPL_NOTOPIC, client->getNick(),
                                      chanName + " :No topic is set"));
        else
            client->sendMsg(makeReply(RPL_TOPIC, client->getNick(),
                                      chanName + " :" + topic));
        return;
    }

    // Set topic
    if (channel->isTopicRestricted() && !channel->isOperator(client))
    {
        client->sendMsg(makeReply(ERR_CHANOPRIVSNEEDED, client->getNick(),
                                  chanName + " :You're not channel operator"));
        return;
    }

    const std::string &newTopic = params[1];
    channel->setTopic(newTopic, client);

    std::string msg = client->getPrefix() + " TOPIC " + chanName
                      + " :" + newTopic + "\r\n";
    channel->broadcast(msg, NULL);
}
