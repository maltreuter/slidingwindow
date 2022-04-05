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
#include <poll.h>
#include <vector>
#include <algorithm>
#include <bitset>

#include "Frame.h"

using namespace std;

typedef struct connection_info {
	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	int packet_size;
	int header_len;
	int window_size;
	int protocol;
	int errors;
	int total_bytes_written;
	int packets_rcvd;
	vector<int> lost_acks;
} connection_info;

class Server {
	public:
		string port;
		int backlog;
		struct addrinfo *address_info;
		int sockfd;

		connection_info conn_info;

		Server(string port, int backlog);
		~Server();
		void start_server();
		void handle_connections();
		int handshake();
		int stop_and_wait(FILE* file);
		int go_back_n(FILE* file);
		int selective_repeat(FILE* file);
		bool check_checksum(string checksum, string data, int blockSize);
		int close_server();
};

#endif
