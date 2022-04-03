#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <stdio.h>
#include <filesystem>

#include "utils.h"

using namespace std;

class Server {
	public:
		string port;
		int backlog;
		struct addrinfo *address_info;
		int sockfd;

		int packet_size;
		int protocol;
		int header_len;
		int errors;

		Server(string port, int backlog);
		~Server();
		void start_server();
		void handle_connections();
		int handshake();
		int runProtocol();
		int close_server();
};

#endif
