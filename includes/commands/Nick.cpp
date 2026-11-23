

#include "Server.hpp"
#include "Client.hpp"
#include "utils.hpp"
#include <iostream>


// NICK <nickname>
// Can arrive before or after USER. Triggers welcome if registration complete.
void cmdNick(Server &srv, Client *client, const std::vector<std::string> &params)
{
    if (!client->hasPasswd())
    {
        client->sendMsg(makeReply(ERR_PASSWDMISMATCH, "*",
                                  ":Password required"));
        return;
    }
    if (params.empty())
    {
        client->sendMsg(makeReply(ERR_NONICKNAMEGIVEN, client->getNick(),
                                  ":No nickname given"));
        return;
    }

    const std::string &newNick = params[0];

    if (!isValidNick(newNick))
    {
        client->sendMsg(makeReply(ERR_ERRONEUSNICKNAME, client->getNick(),
                                  newNick + " :Erroneous nickname"));
        return;
    }

    // Check nick not already taken by another client
    Client *existing = srv.getClientByNick(newNick);
    if (existing && existing != client)
    {
        client->sendMsg(makeReply(ERR_NICKNAMEINUSE, client->getNick(),
                                  newNick + " :Nickname is already in use"));
        return;
    }

    // If already registered, broadcast nick change to all channels
    if (client->isAuthenticated())
    {
        std::string msg = client->getPrefix() + " NICK :" + newNick + "\r\n";
        // Notify all channels this client is in
        std::map<std::string, Channel *> &channels = srv.getChannels();
        for (std::map<std::string, Channel *>::iterator it = channels.begin();
             it != channels.end(); ++it)
        {
            if (it->second->hasMember(client))
                it->second->broadcast(msg, NULL); // include sender
        }
    }

    client->setNick(newNick);

    // Complete registration if USER already received
    if (!client->isAuthenticated() && client->hasUser())
    {
        client->setRegistered();
        sendWelcome(client);
    }
}
