# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/09/27 23:34:05 by rmedeiro          #+#    #+#              #
#    Updated: 2026/10/16 13:57:14 by rmedeiro         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = ircserv
B_NAME = miku

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -MMD -MP
CXXAFLAGS = -std=c++98

DEBUG_FLAGS = -g

OBJ_DIR = .objs
DBG_DIR = .objs-debug
B_OBJ_DIR = .objs-bonus

SRCS = srcs/main.cpp \
	   srcs/Server.cpp \
	   srcs/Client.cpp \
	   srcs/Commands.cpp \
	   srcs/Channels.cpp \
	   srcs/Utils.cpp

B_SRCS = bonus/srcs/Bot.cpp \

OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DBG_OBJS = $(SRCS:%.cpp=$(DBG_DIR)/%.o)

B_OBJS = $(SRCS:%.cpp=$(B_OBJ_DIR)/%.o) \
             $(B_SRCS:%.cpp=$(B_OBJ_DIR)/%.o)

B_DEPS = $(B_OBJS:.o=.d)

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

$(B_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) -D BONUS -Ibonus/includes -c $< -o $@

d: $(DBG_OBJS)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) $(DEBUG_FLAGS) $(DBG_OBJS) -o $(NAME)
	@echo -e "$(CYAN)$(NAME) compiled with debug flags!$(RESET)"

$(DBG_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) $(DEBUG_FLAGS) -c $< -o $@

miku: $(B_NAME)

test:
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) -Ibonus/includes $(B_SRCS) -o test && ./test

$(B_NAME): $(B_OBJS)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) $(B_OBJS) -o $(B_NAME)
	@echo -e "$(GREEN)$(B_NAME) compiled!$(RESET)"
	
clean:
	rm -rf $(OBJ_DIR) $(DBG_DIR) $(B_OBJ_DIR)
	@echo -e "$(YELLOW)Object files removed!$(RESET)"

fclean: clean
	rm -f $(NAME) $(B_NAME) test
	@echo -e "$(RED)Execs removed!$(RESET)"

re: fclean all
	@echo -e "$(GREEN)Recompiled $(NAME)!$(RESET)"

-include $(DEPS) $(DBG_DEPS) $(B_DEPS)

.PHONY: all beyond d clean fclean re
