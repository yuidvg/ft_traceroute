CC = cc

EXEC = ft_traceroute
CFLAGS = -Wall -Werror -Wextra -I include/
CLIBS = -lm
SRCS_DIR=src/
SRCS = $(wildcard $(SRCS_DIR)*.c)
OBJS_DIR=obj/
OBJS = $(SRCS:$(SRCS_DIR)%.c=$(OBJS_DIR)%.o)

DEBUG_EXEC = ft_traceroute_debug.out
DEBUG_CFLAGS = -g
DEBUG_OBJS_DIR = obj_debug/
DEBUG_OBJS = $(SRCS:$(SRCS_DIR)%.c=$(DEBUG_OBJS_DIR)%.o)

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS) $(CLIBS)

$(OBJS_DIR)%.o: $(SRCS_DIR)%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(CLIBS)

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

debug: $(DEBUG_EXEC)

$(DEBUG_EXEC): $(DEBUG_OBJS)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $(DEBUG_EXEC) $(DEBUG_OBJS) $(CLIBS)

$(DEBUG_OBJS_DIR)%.o: $(SRCS_DIR)%.c | $(DEBUG_OBJS_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@ $(CLIBS)

$(DEBUG_OBJS_DIR):
	mkdir -p $(DEBUG_OBJS_DIR)

clean:
	rm -rf $(OBJS_DIR) $(DEBUG_OBJS_DIR)

fclean: clean
	rm -f $(EXEC) $(DEBUG_EXEC)

re: fclean all

.PHONY: clean fclean re debug
