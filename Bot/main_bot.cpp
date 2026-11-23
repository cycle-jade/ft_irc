#include "Bot.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        std::cerr << "Usage: ./ircbot <host> <port> <password>" << std::endl;
        std::cerr << "Example: ./ircbot 127.0.0.1 6667 secret" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[2]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: invalid port." << std::endl;
        return 1;
    }

    try
    {
        Bot bot(argv[1], port, argv[3]);
        bot.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Bot error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}