


#include "Server.hpp"
#include "Client.hpp"
#include "utils.hpp"

// PASS <password>
// Must be sent before NICK/USER. Only valid before registration.
void cmdPass(Server &srv, Client *client, const std::vector<std::string> &params)
{
    if (client->isAuthenticated())
    {
        client->sendMsg(makeReply(ERR_ALREADYREGISTRED, client->getNick(),
                                  ":You may not reregister"));
        return;
    }
    if (params.empty())
    {
        client->sendMsg(makeReply(ERR_NEEDMOREPARAMS, client->getNick(),
                                  "PASS :Not enough parameters"));
        return;
    }
    if (params[0] != srv.getPassword())
    {
        client->sendMsg(makeReply(ERR_PASSWDMISMATCH, client->getNick(),
                                  ":Password incorrect"));
        return;
    }
    client->setPasswd(true);
}
