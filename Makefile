CC = clang -g
CC_FLAGS = -Wall -Werror

main: main.c
	${CC} ${CC_FLAGS} main.c -o main

