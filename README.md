*This project has been created as part of the 42 curriculum by isramire, jpoma-sa, glicciar.*

# ft_irc — Internet Relay Chat Server

## Description

**ft_irc** is a fully functional IRC (Internet Relay Chat) server written in C++98. It follows the IRC protocol (RFC 1459) and allows multiple clients to connect simultaneously, authenticate, join channels, exchange messages, and manage channels using standard IRC commands.

The server is built around a single `poll()` call for non-blocking I/O multiplexing, which allows handling many clients concurrently without threads or forking. A companion IRC bot is also included as a bonus feature.

**Key features:**
- Multi-client support with non-blocking I/O via `poll()`
- Full client registration flow: `PASS`, `NICK`, `USER`
- Channel management: `JOIN`, `PART`, `QUIT`
- Messaging: `PRIVMSG`, `NOTICE`
- Channel operator commands: `KICK`, `INVITE`, `TOPIC`, `MODE`
- Channel modes: `+i` (invite-only), `+t` (topic restriction), `+k` (key/password), `+o` (operator), `+l` (user limit)
- IRC numeric replies compliant with RFC 1459
- IRC Bot with interactive commands (`!roll`, `!flip`, `!8ball`, `!choose`, `!kick`)

## Instructions

### Requirements

- C++98 compatible compiler (`g++` or `clang++`)
- POSIX-compatible OS (Linux or macOS)
- An IRC client to connect and test (e.g., [irssi](https://irssi.org/), [WeeChat](https://weechat.org/), [HexChat](https://hexchat.github.io/))

### Compilation

```bash
# Build both server and bot
make

# Clean object files
make clean

# Full clean (objects + executables)
make fclean

# Rebuild everything
make re
```

### Running the Server

```bash
./ircserv <port> <password>

# Example:
./ircserv 6667 your_server_password
```

- `port`: Port the server listens on (e.g., `6667`)
- `password`: Password that clients must provide to connect

### Running the Bot (Bonus)

```bash
./ircbot <host> <port> <password>

# Example:
./ircbot 127.0.0.1 6667 your_server_password
```

The bot automatically joins `#general` and responds to commands prefixed with `!`:

| Command | Description |
|---------|-------------|
| `!help` | List available commands |
| `!roll NdN` | Roll dice (e.g., `!roll 2d6`) |
| `!flip` | Flip a coin |
| `!8ball <question>` | Magic 8-ball |
| `!choose a \| b \| c` | Random choice |
| `!kick <nick>` | Kick a user (requires operator) |

### Connecting with irssi

```
/connect 127.0.0.1 6667 your_server_password
/nick yournick
/join #general
```

### Quick Test with nc

Per the subject, partial data handling can be tested with:

```bash
nc -C 127.0.0.1 6667
```

Type `com`, press `Ctrl+D`, type `man`, press `Ctrl+D`, type `d` and `Enter`. The server must correctly reconstruct the full command from partial TCP packets.

## Resources

### IRC Protocol References

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 — IRC: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [Modern IRC Client Protocol (ircdocs.horse)](https://modern.ircdocs.horse/)
- [IRC Numerics Reference](https://defs.ircdocs.horse/defs/numerics.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

### How AI Was Used

AI was used during the development of this project in the following ways:

- **Understanding the IRC protocol:** Clarifying RFC 1459 command behavior, numeric reply codes, and registration handshake flow.
- **Debugging assistance:** Diagnosing issues with `poll()` event handling, partial message buffering, and non-blocking socket behavior.
- **Testing guidance:** Generating test scenarios for irssi connections and edge cases (e.g., partial TCP packets, concurrent clients).
- **Code review:** Checking for potential issues in command parsing and error handling logic.

All AI-generated suggestions were reviewed, tested, and adapted by the authors before integration. No code was copied without full understanding of its behavior.
