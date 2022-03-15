#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

class Client {
	public:
		string host;
		string port;

		Client(string host, string port);
		~Client();
		void connect();
		int send_file();
		int receive_file();
};

#endif
