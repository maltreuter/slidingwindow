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
	hints.ai_socktype = SOCK_DGRAM;
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
}

void Server::handle_connections() {
	fd_set readfds;
	struct timeval timeout;

	int bytes_rcvd;
	int bytes_written;
	int bytes_sent;

	/* accept connections */
	int loop = 0;
	while(1) {
		struct sockaddr_storage client_addr;
		socklen_t addr_size = sizeof(client_addr);

		FD_ZERO(&readfds);
		FD_SET(this->sockfd, &readfds);

		/* receive header size as well? header = seq_num + checksum */
		/* receive packet size from user input in the client */
		char packet_size[5];
		bytes_rcvd = recvfrom(this->sockfd, packet_size, 4, 0, (struct sockaddr *) &client_addr, &addr_size);
		if(bytes_rcvd == -1) {
			perror("recvfrom");
			exit(1);
		}

		packet_size[4] = '\0';

		/* open file to write */
		string file_path = "./out" + to_string(loop);
		FILE *file = fopen(file_path.c_str(), "wb");

		int total = 0;
		int packets_rcvd = 0;

		string ack;

		bool write_done = false;

		while(!write_done) {
			char buffer[atoi(packet_size) + 5];
			memset(buffer, 0, atoi(packet_size) + 5);

			bytes_rcvd = recvfrom(this->sockfd, buffer, atoi(packet_size) + 5, 0, (struct sockaddr *) &client_addr, &addr_size);
			if(bytes_rcvd == -1) {
				perror("recvfrom");
				continue;
			}
			cout << "received packet " << packets_rcvd << endl;

			/* client will send "done" when it is finished sending the file */
			/* look for "done" in the buffer to stop looping, otherwise write to file */
			if(string(buffer).find("done") != string::npos) {
				write_done = true;
				cout << "received 'done'" << endl;
			} else {
				/* separate data from 4 byte seq num at end of buffer */
				/* send ack and write buffer to file */
				string data = string(buffer);
				string seq_num;

				// cout << "bytes received: " << bytes_rcvd << endl;

				size_t pos = data.find(":");
				if(pos != string::npos) {
					seq_num = data.substr(pos + 1);
					data.erase(pos, string::npos);
					ack = "ack" + seq_num;
				} else {
					cout << "damaged packet" << endl;
					exit(1);
				}

				bytes_sent = sendto(this->sockfd, ack.c_str(), ack.length(), 0, (struct sockaddr *) &client_addr, addr_size);
				if(bytes_sent == -1) {
					perror("sendto");
					continue;
				}

				cout << "ack " << seq_num << " sent" << endl;

				// cout << data.c_str() << endl;
				bytes_written = fwrite(data.c_str(), 1, data.length(), file);
				// cout << bytes_written << endl;

				total += bytes_written;
				packets_rcvd++;
			}
		}

		fclose(file);

		cout << "packets received: " << packets_rcvd << endl;
		cout << "bytes written: " << total << endl;

		cout << "Client disconnected" << endl;
		loop++;
	}
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
}
