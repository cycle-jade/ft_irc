
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"

// QUIT [:<reason>]
// Client voluntarily disconnects. Broadcast to all shared channels first.
void cmdQuit(Server &srv, Client *client, const std::vector<std::string> &params)
{
    std::string reason = params.empty() ? "Leaving" : params[0];
    std::string msg = client->getPrefix() + " QUIT :" + reason + "\r\n";

    // Notify every channel the client was in
    std::map<std::string, Channel *> &channels = srv.getChannels();
    for (std::map<std::string, Channel *>::iterator it = channels.begin();
         it != channels.end(); ++it)
    {
        if (it->second->hasMember(client))
            it->second->broadcast(msg, client); // don't send to the quitter
    }

    srv.disconnectClient(client->getFd());
}
