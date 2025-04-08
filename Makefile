all: echo-server

flags := -Wall -Wextra -Werror

main.o: main.c
	gcc -c main.c -o main.o

echo-server: main.o
	gcc main.o -o echo-server

clean:
	rm -f main.o
	rm -f echo-server
