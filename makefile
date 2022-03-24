CC    = g++
FLAGS = -Wall -std=c++17

all: client server

client: client.o frame.o
	$(CC) $(FLAGS) Client.o Frame.o -o client

server: server.o
	$(CC) $(FLAGS) Server.o -o server

client.o:
	$(CC) $(FLAGS) -c Client.cpp

server.o:
	$(CC) $(FLAGS) -c Server.cpp

frame.o:
	$(CC) $(FLAGS) -c Frame.cpp

clean:
	rm *.gch *.o client server out
