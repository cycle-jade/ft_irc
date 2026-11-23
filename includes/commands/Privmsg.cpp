

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"

// PRIVMSG <target> :<message>
// target can be a #channel or a nickname
// Also handles NOTICE (same logic, no error replies sent back)
void cmdPrivmsg(Server &srv, Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS, client->getNick(),
                                  "PRIVMSG :Not enough parameters"));
        return;
    }

    const std::string &target = params[0];
    const std::string &text   = params[1];
    std::string msg = client->getPrefix() + " PRIVMSG " + target + " :" + text + "\r\n";

    if (target[0] == '#') // channel message
    {
        std::map<std::string, Channel *> &channels = srv.getChannels();
        std::map<std::string, Channel *>::iterator it = channels.find(target);
        if (it == channels.end())
        {
            client->sendMsg(makeReply(ERR_NOSUCHCHANNEL, client->getNick(),
                                      target + " :No such channel"));
            return;
        }
        Channel *channel = it->second;
        if (!channel->hasMember(client))
        {
            client->sendMsg(makeReply(ERR_CANNOTSENDTOCHAN, client->getNick(),
                                      target + " :Cannot send to channel"));
            return;
        }
        // Broadcast to all members EXCEPT the sender
        channel->broadcast(msg, client);
    }
    else // private message to a nick
    {
        Client *dest = srv.getClientByNick(target);
        if (!dest)
        {
            client->sendMsg(makeReply(ERR_NOSUCHNICK, client->getNick(),
                                      target + " :No such nick"));
            return;
        }
        dest->sendMsg(msg);
    }
}
