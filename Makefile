NAME = ircserv

SRCS = main.cpp server.cpp client.cpp util.cpp channel.cpp
OBJS = $(SRCS:.cpp=.o)
CXX = g++
CXXFLAGS = -std=c++98 -Wall -Werror -Wextra -pedantic -g3 -Wno-unused-parameter -Wno-unused-private-field # -fsanitize=address

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all
