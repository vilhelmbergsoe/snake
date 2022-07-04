CFLAGS=-Wall -Werror -Wextra -pthread -O2

all:
	$(CC) $(CFLAGS) -o snake main.c

run:
	make && ./snake
