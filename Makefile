# Compiler to be used
CXX = c++

# Compiler flags
CXXFLAGS = -std=c++98 -Wall -Werror -Wextra -pedantic -g3 -Wno-unused-parameter -Wno-unused-private-field -fsanitize=address

# Name of the executable
NAME = ircserv

# List of source files (without the .cpp extension)
FILES = main channel client server util

# List of source files with the .cpp extension
SRCS = $(addsuffix .cpp, $(FILES))

# List of object files (with their paths)
OBJS = $(addprefix objs/, $(addsuffix .o, $(FILES)))

# Additional include paths (not used in this Makefile)
# HEADERS =
#INCLUDE_PATHS = -I.

# Rule to compile individual source files into object files
objs/%.o: %.cpp
# Check if the 'objs' directory does not exist, if not, it creates it
		@if ! [ -d objs ]; then\
			mkdir objs;\
		fi
		@$(CXX) $(CXXFLAGS) -c -o $@ $<

# Default target: build the executable
all: $(NAME)

# Target to build the executable
$(NAME): $(HEADERS) $(OBJS)
# Link the object files into the executable
# @$(CXX) $(CXXFLAGS) $(INCLUDE_PATHS) -o $@ $(SRCS)
		@$(CXX) $(CXXFLAGS) -o $@ $(SRCS)
		@echo "\033[0;32m ✅ Compilation done! ✅ \033[0m"

# Target to clean up object files
clean:
# Remove the 'objs' directory and all its contents
		@rm -rf objs
		@echo "\033[0;32m🧹🧼All cleaned!🧹🧼\033[0m"

# Target to perform a clean build
#re: clean
# Rebuild the executable
#       rm -rf $(NAME)
# @echo "\033[0;32m🧹🧼All cleaned!🧹🧼\033[0m"

# Target to clean up object files and the executable
fclean: clean
# Remove the executable
		@rm -rf $(NAME)

# Target to perform a clean build followed by building the executable
re: fclean all

# Declare 'all', 'clean', 'fclean', and 're' as phony targets
.PHONY: all clean fclean re%
