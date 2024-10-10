NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98

all: $(NAME)

 Fixed


clean:
		@rm -rf objs
		@echo "\033[0;32m🧹🧼All cleaned!🧹🧼\033[0m"

fclean: clean
		@rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re

