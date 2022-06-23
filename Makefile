all:
	gcc -Wall -Werror -Wextra -pthread -o snake main.c

run:
	make && ./snake

release:
	musl-gcc -Wall -Werror -Wextra -pthread -Os -s -static -o snake main.c
