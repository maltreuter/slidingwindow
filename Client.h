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
#include <algorithm>
#include <bitset>

#include "Frame.h"
#include "utils.h"

using namespace std;

typedef struct user_input {
	string file_path;
	string host;
	string port;
	int packet_size;
	int timeout_int;
	int window_size;
	int max_seq_num;
	int errors;
	int protocol;
	int ack_len;
	int header_len;
	vector<int> lost_packets;
	vector<int> lost_acks;
	vector<int> corrupt_packets;
} user_input;

class Client {
	public:
		user_input user;
		int sockfd;
		struct sockaddr *server_addr;
		struct addrinfo *address_info;
		socklen_t server_addr_len;

		Client();
		~Client();
		int connect();
		int disconnect();
		int handshake();
		int send_to_server(string send_str);
		Frame getNextFrame(FILE* file, bool* read_done, int seq_num);
		int send_frame(Frame f, bool resend);
		int send_frame_with_errors(Frame f, bool resend, int packets_sent);
};

#endif
