

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include <cstdlib>

// MODE <#channel> <+/-modes> [params...]
//
// Supported modes:
//   i  - invite-only          (no param)
//   t  - topic restricted     (no param)
//   k  - channel key/password (+k <key> | -k)
//   o  - operator privilege   (+o <nick> | -o <nick>)
//   l  - user limit           (+l <limit> | -l)

void cmdMode(Server &srv, Client *client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS, client->getNick(),
                                  "MODE :Not enough parameters"));
        return;
    }

    const std::string &chanName = params[0];

    // MODE #channel with no flags = return current modes
    std::map<std::string, Channel *> &channels = srv.getChannels();
    std::map<std::string, Channel *>::iterator it = channels.find(chanName);
    if (it == channels.end())
    {
        client->sendMsg(makeReply(ERR_NOSUCHCHANNEL, client->getNick(),
                                  chanName + " :No such channel"));
        return;
    }
    Channel *channel = it->second;

    if (params.size() == 1)
    {
        // Just show current modes
        std::string modes = "+";
        if (channel->isInviteOnly())     modes += "i";
        if (channel->isTopicRestricted()) modes += "t";
        if (channel->hasKey())           modes += "k";
        if (channel->hasUserLimit())     modes += "l";
        client->sendMsg(makeReply(RPL_CHANNELMODEIS, client->getNick(),
                                  chanName + " " + modes));
        return;
    }

    // Must be operator to change modes
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

    const std::string &modeStr = params[1];
    size_t paramIdx = 2; // index into params[] for mode arguments
    bool adding = true;

    // Track what we applied to broadcast one MODE reply
    std::string appliedModes = "";
    std::string appliedParams = "";

    for (size_t i = 0; i < modeStr.size(); ++i)
    {
        char m = modeStr[i];

        if (m == '+') { adding = true;  continue; }
        if (m == '-') { adding = false; continue; }

        switch (m)
        {
            case 'i': // invite-only (no param)
                channel->setInviteOnly(adding);
                appliedModes += (adding ? "+i" : "-i");
                break;

            case 't': // topic restricted (no param)
                channel->setTopicRestricted(adding);
                appliedModes += (adding ? "+t" : "-t");
                break;

            case 'k': // channel key
                if (adding)
                {
                    if (paramIdx >= params.size())
                    {
                        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS,
                            client->getNick(), "MODE :Not enough parameters"));
                        break;
                    }
                    channel->setKey(params[paramIdx]);
                    appliedModes += "+k";
                    appliedParams += " " + params[paramIdx];
                    paramIdx++;
                }
                else
                {
                    channel->removeKey();
                    appliedModes += "-k";
                }
                break;

            case 'o': // operator privilege
            {
                if (paramIdx >= params.size())
                {
                    client->sendMsg(makeReply(ERR_NEEDMOREPARAMS,
                        client->getNick(), "MODE :Not enough parameters"));
                    break;
                }
                const std::string &targetNick = params[paramIdx++];
                Client *target = srv.getClientByNick(targetNick);
                if (!target || !channel->hasMember(target))
                {
                    client->sendMsg(makeReply(ERR_USERNOTINCHANNEL,
                        client->getNick(),
                        targetNick + " " + chanName + " :They aren't on that channel"));
                    break;
                }
                if (adding)  channel->addOperator(target);
                else         channel->removeOperator(target);
                appliedModes += (adding ? "+o" : "-o");
                appliedParams += " " + targetNick;
                break;
            }

            case 'l': // user limit
                if (adding)
                {
                    if (paramIdx >= params.size())
                    {
                        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS,
                            client->getNick(), "MODE :Not enough parameters"));
                        break;
                    }
                    int limit = std::atoi(params[paramIdx].c_str());
                    if (limit > 0)
                    {
                        channel->setUserLimit(limit);
                        appliedModes += "+l";
                        appliedParams += " " + params[paramIdx];
                    }
                    paramIdx++;
                }
                else
                {
                    channel->removeUserLimit();
                    appliedModes += "-l";
                }
                break;

            default:
                client->sendMsg(makeReply(ERR_UNKNOWNMODE, client->getNick(),
                                          std::string(1, m) + " :is unknown mode char"));
                break;
        }
    }

    // Broadcast the applied mode changes to everyone in the channel
    if (!appliedModes.empty())
    {
        std::string modeMsg = client->getPrefix() + " MODE " + chanName
                              + " " + appliedModes + appliedParams + "\r\n";
        channel->broadcast(modeMsg, NULL);
    }
}
