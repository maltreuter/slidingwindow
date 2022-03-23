#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <fstream>
#include <filesystem>

using namespace std;

class Client {
	public:
		string host;
		string port;
		int sockfd;

		Client(string host, string port);
		~Client();
		void connect();
		int send_file(string file_path);
		int receive_file();
		int send_cin();
};

#endif
