CC    = g++
FLAGS = -Wall -std=c++14

all: client server

client: client.o
	$(CC) $(FLAGS) Client.o -o client

server: server.o
	$(CC) $(FLAGS) Server.o -o server

client.o:
	$(CC) $(FLAGS) -c Client.cpp

server.o:
	$(CC) $(FLAGS) -c Server.cpp

clean:
	rm *.gch *.o client server
