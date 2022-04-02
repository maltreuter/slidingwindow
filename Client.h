#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <filesystem>
#include <stdio.h>
#include <vector>

#include "Frame.h"

using namespace std;

typedef struct header {
	string packet_size;
	string max_seq_num;
	string file_md5;
} header;

class Client {
	public:
		string host;
		string port;
		int sockfd;
		struct sockaddr *server_addr;
		socklen_t server_addr_len;

		Client(string host, string port);
		~Client();
		void connect();
		int send_cin();
		int disconnect();
};

#endif
