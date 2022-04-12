CC    = g++
FLAGS = -Wall -std=c++17
POSEIDON = -lstdc++fs

all: client server

server: server.o frame.o utils.o
	$(CC) $(FLAGS) Server.o Frame.o utils.o -o server

client: menu.o frame.o client.o stopandwait.o gobackn.o selectiverepeat.o utils.o
	$(CC) $(FLAGS) Menu.o Frame.o Client.o StopAndWait.o GoBackN.o SelectiveRepeat.o utils.o -o client $(POSEIDON)

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

utils.o:
	$(CC) $(FLAGS) -c utils.cpp

clean:
	rm *.gch *.o client server out*
