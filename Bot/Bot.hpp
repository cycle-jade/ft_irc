#ifndef BOT_HPP
#define BOT_HPP

#include <string>
#include <vector>
#include <ctime>

#define BOT_NICK     "ChanBot"
#define BOT_USER     "chanbot"
#define BOT_REALNAME "ChanBot v1.0 - type !help"
#define BOT_CHANNEL  "#general"
#define RECV_BUFSIZE  512

class Bot
{
public:
    Bot(const std::string &host, int port, const std::string &password);
    ~Bot();

    void run();

private:
    // --- Connection ---
    std::string  _host;
    int          _port;
    std::string  _password;
    int          _fd;
    std::string  _recvBuf;
    time_t       _startTime;

    // --- Setup ---
    void _connect();
    void _register();

    // --- Event loop ---
    void _loop();
    void _receive();

    // --- IRC protocol helpers ---
    void _send(const std::string &msg);
    void _sendPrivmsg(const std::string &target, const std::string &text);
    void _handleLine(const std::string &line);
    void _handlePing(const std::string &line);
    void _handlePrivmsg(const std::string &prefix,
                        const std::string &target,
                        const std::string &text);

    // --- Command handlers ---
    void _cmdHelp   (const std::string &target);
    void _cmdRoll   (const std::string &target, const std::string &args);
    void _cmdFlip   (const std::string &target);
    void _cmd8ball  (const std::string &target, const std::string &question);
    void _cmdChoose (const std::string &target, const std::string &args);
    void _cmdKick   (const std::string &channel, const std::string &args,
                     const std::string &requesterNick);
    void _cmdUptime (const std::string &target);

    // --- Helpers ---
    std::string _nickFromPrefix(const std::string &prefix) const;
    std::vector<std::string> _split(const std::string &s, char delim) const;
    std::string _trim(const std::string &s) const;

    // Non-copyable
    Bot(const Bot &);
    Bot &operator=(const Bot &);
};

#endif