CC    = g++
FLAGS = -Wall -std=c++17
POSEIDON = -lstdc++fs

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

udp: clientudp serverudp

clientudp: clientudp.o frame.o
	$(CC) $(FLAGS) ClientUDP.o Frame.o -o clientudp

serverudp: serverudp.o
	$(CC) $(FLAGS) ServerUDP.o -o serverudp

clientudp.o:
	$(CC) $(FLAGS) -c ClientUDP.cpp

serverudp.o:
	$(CC) $(FLAGS) -c ServerUDP.cpp

clean:
	rm *.gch *.o client server clientudp serverudp out*
