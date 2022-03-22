#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

using namespace std;

class Server {
	public:
		string port;
		int backlog;
		struct addrinfo *address_info;

		Server(string port, int backlog);
		~Server();
		void start();
		int send_file();
		int receive_file();
};

#endif
