NAME		= ircserv
BOT_NAME	= ircbot

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98

INCLUDES	= -I includes

# ─── Server sources ───────────────────────────────────────────────────────────
SRCS		= main.cpp \
			  src/Server.cpp \
			  src/Client.cpp \
			  src/Channel.cpp \
			  includes/commands/Pass.cpp \
			  includes/commands/Nick.cpp \
			  includes/commands/User.cpp \
			  includes/commands/Join.cpp \
			  includes/commands/Part.cpp \
			  includes/commands/Quit.cpp \
			  includes/commands/Privmsg.cpp \
			  includes/commands/Kick.cpp \
			  includes/commands/Invite.cpp \
			  includes/commands/Topic.cpp \
			  includes/commands/Mode.cpp

OBJS		= $(SRCS:.cpp=.o)

# ─── Bot sources ──────────────────────────────────────────────────────────────
BOT_SRCS	= Bot/main_bot.cpp \
			  Bot/Bot.cpp

BOT_OBJS	= $(BOT_SRCS:.cpp=.o)

# ─── Rules ────────────────────────────────────────────────────────────────────
all: $(NAME) $(BOT_NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(BOT_NAME): $(BOT_OBJS)
	$(CXX) $(CXXFLAGS) $(BOT_OBJS) -o $(BOT_NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I Bot -c $< -o $@

clean:
	rm -f $(OBJS) $(BOT_OBJS)

fclean: clean
	rm -f $(NAME) $(BOT_NAME)

re: fclean all

.PHONY: all clean fclean re