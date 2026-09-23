NAME = ircserv

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -MMD -MP
CXXAFLAGS = -std=c++98

DEBUG_FLAGS = -g

OBJ_DIR = .objs
DBG_DIR = .objs-debug

SRCS = main.cpp

OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DBG_OBJS = $(SRCS:%.cpp=$(DBG_DIR)/%.o)

DEPS = $(OBJS:.o=.d)
DBG_DEPS = $(DBG_OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) -c $< -o $@

d: $(DBG_OBJS)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) $(DEBUG_FLAGS) $(DBG_OBJS) -o $(NAME)

$(DBG_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXAFLAGS) $(DEBUG_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(DBG_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(DEPS) $(DBG_DEPS)

.PHONY: all d clean fclean re
