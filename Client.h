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
#include <stdlib.h>
#include "Frame.h"

using namespace std;

typedef struct user_input {
	string file_path;
	string host;
	string port;
	int packet_size;
	int timeout_int;
	int window_size;
	int errors;
	int protocol;
	int header_len;
} user_input;

class Client {
	public:
		user_input user;
		int sockfd;
		struct sockaddr *server_addr;
		socklen_t server_addr_len;

		Client();
		~Client();
		int connect();
		int disconnect();
		int handshake();
		int get_current_time();
		Frame getNextFrame(FILE* file, bool* read_done, int packets_sent);
		int send_frame(Frame f);
};

#endif
