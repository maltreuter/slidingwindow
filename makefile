CC    = g++
FLAGS = -Wall -std=c++17
POSEIDON = -lstdc++fs

all: stopandwait server

server: server.o
	$(CC) $(FLAGS) Server.o -o server

stopandwait: client.o frame.o stopandwait.o
	$(CC) $(FLAGS) Client.o Frame.o StopAndWait.o -o stopandwait

client.o:
	$(CC) $(FLAGS) -c Client.cpp

server.o:
	$(CC) $(FLAGS) -c Server.cpp

frame.o:
	$(CC) $(FLAGS) -c Frame.cpp

stopandwait.o:
	$(CC) $(FLAGS) -c StopAndWait.cpp

clean:
	rm *.gch *.o client server out* stopandwait
