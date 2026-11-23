#include "Server.hpp"
#include "Client.hpp"
#include "utils.hpp"

// USER <username> <hostname> <servername> :<realname>
// Sets username and realname. Only valid before registration.
void cmdUser(Server &srv, Client *client, const std::vector<std::string> &params)
{
    (void)srv;
    if (client->isAuthenticated())
    {
        client->sendMsg(makeReply(ERR_ALREADYREGISTRED, client->getNick(),
                                  ":You may not reregister"));
        return;
    }
    if (params.size() < 4)
    {
        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS, client->getNick(),
                                  "USER :Not enough parameters"));
        return;
    }

    client->setUsername(params[0]);
    // params[3] is the realname (may start with ':' already stripped by parser)
    client->setRealname(params[3]);

    // Complete registration if NICK already received
    if (!client->isAuthenticated() && client->hasNick())
    {
        client->setRegistered();
        sendWelcome(client);
    }
}
