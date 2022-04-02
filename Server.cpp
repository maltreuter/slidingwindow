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

		/* receive packet size from user input in the client */
		char packet_size[8];
		bytes_rcvd = recvfrom(this->sockfd, packet_size, 7, 0, (struct sockaddr *) &client_addr, &addr_size);
		if(bytes_rcvd == -1) {
			perror("recvfrom");
			exit(1);
		}
		packet_size[bytes_rcvd] = '\0';

		/* receive header length */
		char header_len[8];
		bytes_rcvd = recvfrom(this->sockfd, header_len, 7, 0, (struct sockaddr *) &client_addr, &addr_size);
		if(bytes_rcvd == -1) {
			perror("recvfrom");
			exit(1);
		}
		header_len[bytes_rcvd] = '\0';

		/* open file to write */
		string file_path = "./out" + to_string(loop);
		FILE *file = fopen(file_path.c_str(), "wb");

		int total = 0;
		int packets_rcvd = 0;

		string ack;

		bool write_done = false;

		int lost_ack_count = 0;

		while(!write_done) {
			char buffer[atoi(packet_size) + atoi(header_len) + 1];
			memset(buffer, 0, atoi(packet_size) + atoi(header_len) + 1);

			bytes_rcvd = recvfrom(this->sockfd, buffer, atoi(packet_size) + atoi(header_len) + 1, 0, (struct sockaddr *) &client_addr, &addr_size);
			if(bytes_rcvd == -1) {
				perror("recvfrom");
				continue;
			}
			cout << "received packet " << packets_rcvd << endl;
			// cout << "sizeof buffer: " << sizeof(buffer) << endl;

			/* parse out header */
			unsigned char header[atoi(header_len) + 1];
			unsigned char data[atoi(packet_size)];
			memcpy(header, buffer, atoi(header_len) + 1);
			memcpy(data, buffer + atoi(header_len) + 1, atoi(packet_size));

			string header_s(header, header + sizeof(header));
			// cout << "header: " << header_s << " header size: " << header_s.length() << endl;

			/* client will send "done" when it is finished sending the file */
			/* look for "done" in the buffer to stop looping, otherwise write to file */
			if(header_s.find("done") != string::npos) {
				write_done = true;
				cout << "received 'done'" << endl;
			} else {
				/* separate data from 8 byte seq num */
				/* send ack and write buffer to file */
				ack = "ack" + header_s;
				// cout << "bytes received: " << bytes_rcvd << endl;

				//this should "lose" ack0020 twice and ack0050 once
				// if((ack == "ack0020" && lost_ack_count < 2) || (ack == "ack0050" && lost_ack_count < 1)) {
				// 	cout << ack << " not sent" << endl;
				// 	lost_ack_count++;
				// } else {
				bytes_sent = sendto(this->sockfd, ack.c_str(), ack.length(), 0, (struct sockaddr *) &client_addr, addr_size);
				if(bytes_sent == -1) {
					perror("sendto");
					continue;
				}

				cout << "ack " << header_s << " sent" << endl;

				// cout << data.c_str() << endl;
				/* strip null terminator and write */
				bytes_written = fwrite(data, 1, bytes_rcvd - sizeof(header), file);
				// if(data.length() > (size_t)atoi(packet_size)) {
				// 	bytes_written = fwrite(data.data(), 1, data.length() - 1, file);
				// } else {
				// 	bytes_written = fwrite(data.data(), 1, data.length(), file);
				// }

				total += bytes_written;
				packets_rcvd++;
				lost_ack_count = 0;
				// }
			}
		}

		fclose(file);

		string out_md5 = get_md5(filesystem::path(file_path));

		cout << "\n************************************" << endl;
		cout << "received file: '" << file_path << "'\tmd5 sum: " << out_md5 << endl;
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
