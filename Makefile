CC=cc
CFLAGS=-Wall -Werror -Wextra -pthread

all:
	$(CC) $(CFLAGS) -o snake main.c

run:
	make && ./snake
