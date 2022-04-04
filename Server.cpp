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
	/* accept connections */
	int loop = 0;
	while(1) {
		handshake();

		string file_path = "./out" + to_string(loop);
		FILE *file = fopen(file_path.c_str(), "wb");

		// stop_and_wait(file);
		go_back_n(file);

		fclose(file);

		// string out_md5 = get_md5(filesystem::path(file_path));

		cout << "\n************************************" << endl;
		cout << "received file: '" << file_path << endl;
		cout << "packets received: " << this->conn_info.packets_rcvd << endl;
		cout << "bytes written: " << this->conn_info.total_bytes_written << endl;

		cout << "Client disconnected" << endl;
		loop++;
	}
}

int Server::handshake() {
	struct sockaddr_storage client_addr;
	this->conn_info = {
		client_addr,
		sizeof(client_addr),
		0,
		0,
		0,
		0,
		0
	};

	/* receive packet size from user input in the client */
	char packet_size[8];
	int bytes_rcvd = recvfrom(this->sockfd, packet_size, 7, 0, (struct sockaddr *) &this->conn_info.client_addr, &this->conn_info.addr_size);
	if(bytes_rcvd == -1) {
		perror("recvfrom");
		exit(1);
	}
	packet_size[bytes_rcvd] = '\0';

	this->conn_info.packet_size = atoi(packet_size);

	/* receive header length */
	char header_len[8];
	bytes_rcvd = recvfrom(this->sockfd, header_len, 7, 0, (struct sockaddr *) &this->conn_info.client_addr, &this->conn_info.addr_size);
	if(bytes_rcvd == -1) {
		perror("recvfrom");
		exit(1);
	}
	header_len[bytes_rcvd] = '\0';

	this->conn_info.header_len = atoi(header_len);

	/* Receive protocol and errors */
	return 0;
}

int Server::stop_and_wait(FILE* file) {
	/* open file to write */
	string ack;

	bool write_done = false;

	int lost_ack_count = 0;

	int bytes_rcvd;
	int bytes_written;
	int bytes_sent;

	while(!write_done) {
		char buffer[this->conn_info.packet_size + this->conn_info.header_len + 1];
		memset(buffer, 0, this->conn_info.packet_size + this->conn_info.header_len + 1);

		bytes_rcvd = recvfrom(this->sockfd,
			buffer,
			this->conn_info.packet_size + this->conn_info.header_len + 1,
			0,
			(struct sockaddr *) &this->conn_info.client_addr,
			&this->conn_info.addr_size
		);
		if(bytes_rcvd == -1) {
			perror("recvfrom");
			continue;
		}
		cout << "received packet " << this->conn_info.packets_rcvd << endl;
		// cout << "sizeof buffer: " << sizeof(buffer) << endl;

		/* parse out header */
		unsigned char header[this->conn_info.header_len + 1];
		unsigned char data[this->conn_info.packet_size];
		memcpy(header, buffer, this->conn_info.header_len + 1);
		memcpy(data, buffer + this->conn_info.header_len + 1, this->conn_info.packet_size);

		string header_s(header, header + sizeof(header));
		// cout << "header: " << header_s << " header size: " << header_s.length() << endl;

		/* client will send "done" when it is finished sending the file */
		/* look for "done" in the buffer to stop looping, otherwise write to file */
		if(header_s.find("done") != string::npos) {
			write_done = true;
			cout << "received 'done'" << endl;
		} else {
			/* send ack and write buffer to file */
			ack = "ack" + header_s;
			// cout << "bytes received: " << bytes_rcvd << endl;

			int seq_num = stoi(header_s);
			// this should "lose" ack 20 and ack 50
			if((seq_num == 20 && lost_ack_count < 1) || (seq_num == 50 && lost_ack_count < 1)) {
				cout << ack << " not sent" << endl;
				lost_ack_count++;
			} else {
				bytes_sent = sendto(this->sockfd,
					ack.c_str(),
					ack.length(),
					0,
					(struct sockaddr *) &this->conn_info.client_addr,
					this->conn_info.addr_size
				);
				if(bytes_sent == -1) {
					perror("sendto");
					continue;
				}

				cout << "ack " << header_s << " sent" << endl;

				bytes_written = fwrite(data, 1, bytes_rcvd - sizeof(header), file);

				this->conn_info.total_bytes_written += bytes_written;
				this->conn_info.packets_rcvd++;
				lost_ack_count = 0;
			}
		}
	}

	return 0;
}

int Server::go_back_n(FILE* file) {
	string ack;
	bool write_done = false;
	int expected_seq_num = 0;
	string last_ack = "";

	int bytes_rcvd;
	int bytes_written;
	int bytes_sent;

	int lost_ack_count = 0;

	while(!write_done) {
		char buffer[this->conn_info.packet_size + this->conn_info.header_len + 1];
		memset(buffer, 0, this->conn_info.packet_size + this->conn_info.header_len + 1);

		bytes_rcvd = recvfrom(this->sockfd,
			buffer,
			this->conn_info.packet_size + this->conn_info.header_len + 1,
			0,
			(struct sockaddr *) &this->conn_info.client_addr,
			&this->conn_info.addr_size
		);
		if(bytes_rcvd == -1) {
			perror("recvfrom");
			continue;
		}
		// cout << "sizeof buffer: " << sizeof(buffer) << endl;

		/* parse out header */
		unsigned char header[this->conn_info.header_len + 1];
		unsigned char data[this->conn_info.packet_size];
		memcpy(header, buffer, this->conn_info.header_len + 1);
		memcpy(data, buffer + this->conn_info.header_len + 1, this->conn_info.packet_size);

		string header_s(header, header + sizeof(header));
		ack = "ack" + header_s;
		// cout << "header: " << header_s << " header size: " << header_s.length() << endl;

		/* client will send "done" when it is finished sending the file */
		/* look for "done" in the buffer to stop looping, otherwise write to file */
		if(header_s.find("done") != string::npos) {
			write_done = true;
			cout << "received 'done'" << endl;
		} else {
			cout << "received packet " << header_s << endl;
			int seq_num = stoi(header_s);
			if((seq_num == 20 && lost_ack_count < 1) || (seq_num == 50 && lost_ack_count < 1)) {
				cout << ack << " not sent" << endl;
				lost_ack_count++;
			} else {
				/* send ack and write buffer to file */
				// cout << "bytes received: " << bytes_rcvd << endl;

				if(seq_num == expected_seq_num) {
					bytes_sent = sendto(this->sockfd,
						ack.c_str(),
						ack.length(),
						0,
						(struct sockaddr *) &this->conn_info.client_addr,
						this->conn_info.addr_size
					);
					if(bytes_sent == -1) {
						perror("sendto");
						continue;
					}

					cout << "sent ack " << header_s << endl;

					bytes_written = fwrite(data, 1, bytes_rcvd - sizeof(header), file);

					this->conn_info.total_bytes_written += bytes_written;
					this->conn_info.packets_rcvd++;
					expected_seq_num++;
					last_ack = ack;

					lost_ack_count = 0;
				} else {
					bytes_sent = sendto(this->sockfd,
						last_ack.c_str(),
						last_ack.length(),
						0,
						(struct sockaddr *) &this->conn_info.client_addr,
						this->conn_info.addr_size
					);
					if(bytes_sent == -1) {
						perror("sendto");
						continue;
					}
				}
			}
		}

	}

	return 0;
}

int Server::selective_repeat() {
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

	/* signal handler for ctrl-c */
	s.close_server();
}
