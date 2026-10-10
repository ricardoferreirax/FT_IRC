# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/09/27 23:34:05 by rmedeiro          #+#    #+#              #
#    Updated: 2026/10/10 14:36:13 by rmedeiro         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = ircserv

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -MMD -MP
CXXAFLAGS = -std=c++98

DEBUG_FLAGS = -g

OBJ_DIR = .objs
DBG_DIR = .objs-debug

SRCS = srcs/main.cpp \
	   srcs/Server.cpp \
	   srcs/Client.cpp \
	   srcs/Commands.cpp \
	   srcs/Utils.cpp

OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DBG_OBJS = $(SRCS:%.cpp=$(DBG_DIR)/%.o)

DEPS = $(OBJS:.o=.d)
DBG_DEPS = $(DBG_OBJS:.o=.d)

RESET = \033[0m
GREEN = \033[0;32m
RED = \033[0;31m
YELLOW = \033[0;33m
CYAN = \033[0;36m

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) $(OBJS) -o $(NAME)
	@echo -e "$(GREEN)$(NAME) compiled!$(RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) -c $< -o $@

d: $(DBG_OBJS)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) $(DEBUG_FLAGS) $(DBG_OBJS) -o $(NAME)
	@echo -e "$(CYAN)$(NAME) compiled with debug flags!$(RESET)"

$(DBG_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) $(DEBUG_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(DBG_DIR)
	@echo -e "$(YELLOW)Object files removed!$(RESET)"

fclean: clean
	rm -f $(NAME)
	@echo -e "$(RED)$(NAME) removed!"
	@echo -e "$(RED)$(NAME) removed!$(RESET)"

re: fclean all
	@echo -e "$(GREEN)Recompiled $(NAME)!$(RESET)"

-include $(DEPS) $(DBG_DEPS)

.PHONY: all d clean fclean re
