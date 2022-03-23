#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <fstream>

using namespace std;

class Server {
	public:
		string port;
		int backlog;
		struct addrinfo *address_info;
		int sockfd;

		Server(string port, int backlog);
		~Server();
		void start_server();
		void handle_connections();
		int send_file();
		int receive_file();
		int close_server();
};

#endif
