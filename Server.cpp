#include "Server.h"

using namespace std;

Server::Server(string port, int backlog) {
	this->port = port;
	this->backlog = backlog;
}

Server::~Server() {

}

void Server::start() {
	cout << "starting server at 127.0.0.1:" << this->port << endl;

	struct addrinfo hints, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;

	getaddrinfo(NULL, this->port.c_str(), &hints, &this->address_info);

	int yes = 1;
	int sockfd;
	for(p = this->address_info; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(sockfd == -1) {
			perror("socket");
			continue;
		}

		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if(::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

		break;
	}

	listen(sockfd, this->backlog);

	struct sockaddr_storage client_addr;
	socklen_t sin_size;
	int clientfd;
	while(1) {
		sin_size = sizeof(client_addr);
		clientfd = accept(sockfd, (struct sockaddr *) &client_addr, &sin_size);
		send(clientfd, "Hello, world!", 13, 0);
		close(clientfd);
	}
}

int Server::send_file() {
	cout << "Send file" << endl;
	return 0;
}

int Server::receive_file() {
	cout << "Receive file" << endl;
	return 0;
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		cout << "Usage: ./server <port>" << endl;
		exit(1);
	}

	Server s = Server(argv[1], 10);
	s.start();
}
