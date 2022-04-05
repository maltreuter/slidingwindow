CC    = g++
FLAGS = -Wall -std=c++17
POSEIDON = -lstdc++fs

all: client server

server: server.o frame.o
	$(CC) $(FLAGS) Server.o Frame.o -o server

client: menu.o frame.o client.o stopandwait.o gobackn.o selectiverepeat.o
	$(CC) $(FLAGS) Menu.o Frame.o Client.o StopAndWait.o GoBackN.o SelectiveRepeat.o -o client

menu.o:
	$(CC) $(FLAGS) -c Menu.cpp

client.o:
	$(CC) $(FLAGS) -c Client.cpp

server.o:
	$(CC) $(FLAGS) -c Server.cpp

frame.o:
	$(CC) $(FLAGS) -c Frame.cpp

stopandwait.o:
	$(CC) $(FLAGS) -c StopAndWait.cpp

gobackn.o:
	$(CC) $(FLAGS) -c GoBackN.cpp

selectiverepeat.o:
	$(CC) $(FLAGS) -c SelectiveRepeat.cpp

clean:
	rm *.gch *.o client server out*
