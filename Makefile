CC = gcc -Wall
all: server.c client.c
	$(CC) -lpthread -lsocket -lnsl server.c -o server
	$(CC) -lsocket -lnsl client.c -o client
runserver: server
	./server 25020
runclient: client
	./client localhost 25020
clean:
	rm -f server client server.o client.o
linux: server.c client.c
	$(CC) -pthread server.c -o server
	$(CC) client.c -o client
