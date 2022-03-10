#include "Client.h"

using namespace std;

Client::Client(string host, string port) {
	this->host = host;
	this->port = port;
}

Client::~Client() {

}

void Client::connect() {
	char buf[1024];
	int sockfd, nbytes;

	struct addrinfo hints, *address_info, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(this->host.c_str(), this->port.c_str(), &hints, &address_info);

	for(p = address_info; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(sockfd == -1) {
			perror("socket");
			continue;
		}
		if(::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("connect");
			continue;
		}
		break;
	}

	nbytes = recv(sockfd, buf, 1023, 0);
	buf[nbytes] = '\0';

	cout << "Received: '" << buf << "'" << endl;

	close(sockfd);
}

int Client::send_file() {
	cout << "Send file" << endl;
	return 0;
}

int Client::receive_file() {
	cout << "Receive file" << endl;
	return 0;
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		cout << "Usage: ./client <host> <port>" << endl;
		exit(1);
	}

	Client c = Client(argv[1], argv[2]);
	c.connect();

}
