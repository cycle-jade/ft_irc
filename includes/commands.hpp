#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <vector>
#include <string>

class Server;
class Client;

void cmdPass(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdNick(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdUser(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdJoin(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdPart(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdQuit(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdPrivmsg(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdKick(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdInvite(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdTopic(Server &srv, Client *client, const std::vector<std::string> &params);
void cmdMode(Server &srv, Client *client, const std::vector<std::string> &params);

#endif
