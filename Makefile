NAME = codexion

CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread

SRCS = coders/main.c \
       coders/routin.c \
       coders/monitor.c \
       coders/dongles.c \
       coders/heap.c \
       coders/queue.c \
       coders/check_and_set.c \
       coders/init.c \
       coders/parsing.c \
       coders/utils.c

OBJS = $(SRCS:.c=.o)

all: $(NAME)

sanitize: $(OBJS)
	$(CC) -fsanitize=thread $(CFLAGS) $(OBJS) -o $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c coders/codexion.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all sanitize clean fclean re
