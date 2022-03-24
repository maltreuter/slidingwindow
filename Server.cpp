#include "Server.h"

using namespace std;

Server::Server(string port, int backlog) {
	this->port = port;
	this->backlog = backlog;
	this->sockfd = -1;
}

Server::~Server() {

}

void Server::start_server() {
	cout << "starting server at 127.0.0.1:" << this->port << endl;

	struct addrinfo hints, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;

	getaddrinfo(NULL, this->port.c_str(), &hints, &this->address_info);

	int yes = 1;
	for(p = this->address_info; p != NULL; p = p->ai_next) {
		this->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(this->sockfd == -1) {
			perror("socket");
			continue;
		}

		if(setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if(::bind(this->sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(this->sockfd);
			perror("bind");
			continue;
		}

		break;
	}

	if(listen(this->sockfd, this->backlog) == -1) {
		perror("listen");
	}
}

void Server::handle_connections() {
	struct sockaddr_storage client_addr;
	socklen_t sin_size;
	int clientfd;
	int n_bytes;

	while(1) {
		sin_size = sizeof(client_addr);
		clientfd = accept(this->sockfd, (struct sockaddr *) &client_addr, &sin_size);
		if(clientfd == -1) {
			perror("accept");
		}
		cout << "Client connected" << endl;

		FILE *file = fopen("./out", "wb");

		int n_frames;
		int packet_size;

		recv(clientfd, &n_frames, sizeof(int), 0);
		recv(clientfd, &packet_size, sizeof(int), 0);

		int total = 0;
		int packets_rcvd = 0;

		char buffer[packet_size];
		int bytes_written;
		bool write_done = false;

		while(!write_done) {
			n_bytes = recv(clientfd, buffer, packet_size + 4, 0);
			cout << "received packet " << packets_rcvd << endl;

			bytes_written = fwrite(buffer, 1, n_bytes, file);

			total += n_bytes;
			packets_rcvd++;

			if(bytes_written < packet_size) {
				write_done = true;
			}
		}

		close(clientfd);
		fclose(file);

		cout << "packets received: " << packets_rcvd << endl;
		cout << "bytes received: " << total << endl;

		cout << "Client disconnected" << endl;
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

int Server::close_server() {
	return close(this->sockfd);
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		cout << "Usage: ./server <port>" << endl;
		exit(1);
	}

	Server s = Server(argv[1], 10);
	s.start_server();
	s.handle_connections();
	s.close_server();


	// recv cin
	// bool exit = 0;
	// while(!exit) {
	// 	nbytes = recv(clientfd, buf, 1023, 0);
	// 	buf[nbytes] = '\0';
	// 	cout << "Client: " << buf << endl;
	// 	if(strcmp(buf, "exit") == 0) {
	// 		exit = 1;
	// 	}
	// }
}
