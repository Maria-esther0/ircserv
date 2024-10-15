CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98

NAME = ircserv

all: $(NAME)

FILES = src/main 

SRCS = $(addsuffix .cpp, $(FILES))

OBJS = $(addprefix objs/, $(addsuffix .o, $(FILES)))

objs/%.o: %.cpp
		@if ! [ -d $(dir $@) ]; then\
			mkdir -p $(dir $@);\
		fi
		@$(CXX) $(CXXFLAGS) -c -o $@ $<

# @if ! [ -d objs ]; then\
# 	mkdir objs;\
# fi
# @$(CXX) $(CXXFLAGS) -c -o $@ $<


$(NAME): $(HEADERS) $(OBJS)
		@$(CXX) $(CXXFLAGS) -o $@ $(SRCS)
		@echo "\033[0;32m ✅ Compilation done! ✅ \033[0m"

clean:
		@rm -rf objs
		@echo "\033[0;32m🧹🧼All cleaned!🧹🧼\033[0m"

fclean: clean
		@rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re

