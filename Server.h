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
#include <csignal>

#include "Frame.h"
#include "utils.h"

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
	int original_packets;
	int last_seq_num;
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
		int handle_connections(int n_loops);
		int handshake();
		int stop_and_wait(FILE* file);
		int go_back_n(FILE* file);
		int selective_repeat(FILE* file);
		bool check_checksum(string checksum, unsigned char *data, int dataLength, int blockSize);
		int close_server();
};

#endif
