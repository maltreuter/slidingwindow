#include "Server.h"

using namespace std;

bool running = true;

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
	freeaddrinfo(this->address_info);
}

int Server::handle_connections(int n_loops) {
	/* accept connections */
	struct pollfd fds[1];
	fds[0].fd = this->sockfd;
	fds[0].events = POLLIN;

	if(poll(fds, 1, 0) == 0) {
		return -1;
	}

	handshake();

	string file_path = "/tmp/menterzj3144out" + to_string(n_loops);
	FILE *file = fopen(file_path.c_str(), "wb");

	if(this->conn_info.protocol == 0) {
		stop_and_wait(file);
	} else if(this->conn_info.protocol == 1) {
		go_back_n(file);
	} else if(this->conn_info.protocol == 2) {
		selective_repeat(file);
	}

	fclose(file);

	cout << "\n************************************" << endl;
	cout << "Wrote to file: '" << file_path << "'"<< endl;
	cout << "Last packet seq# received: " << this->conn_info.last_seq_num << endl;
	cout << "Number of original packets received: " << this->conn_info.original_packets << endl;
	cout << "Number of retransmitted packets received: " << this->conn_info.packets_rcvd - this->conn_info.original_packets << endl;
	cout << "Number of packets received: " << this->conn_info.packets_rcvd << endl;
	cout << "Number of bytes written: " << this->conn_info.total_bytes_written << endl;
	cout << "md5sum: " << get_md5(file_path) << endl;

	cout << "Client disconnected" << endl;

	return 0;
}

int Server::handshake() {
	struct sockaddr_storage client_addr = {0};
	this->conn_info = {
		client_addr,
		sizeof(client_addr),
		0, /* packet_size */
		0, /* header_len */
		0, /* window_size */
		0, /* max_seq_num */
		0, /* protocol */
		0, /* errors */
		0, /* total_bytes_written */
		0, /* packets_rcvd */
		0, /* original_packets */
		-1 /* last_seq_num */
	};

	/* receive packet size */
	this->conn_info.packet_size = stoi(receive_string());

	/* receive header length */
	this->conn_info.header_len = stoi(receive_string());

	/* receive protocol */
	this->conn_info.protocol = stoi(receive_string());

	/* Receive errors */
	this->conn_info.errors = stoi(receive_string());

	if(this->conn_info.errors == 1) {
		this->conn_info.lost_acks = string_to_vector(receive_string());
	}

	/* Receive window size */
	this->conn_info.window_size = stoi(receive_string());

	this->conn_info.max_seq_num = stoi(receive_string());

	return 0;
}

string Server::receive_string() {
	char buff[1024];
	memset(buff, 0, sizeof(buff));

	int bytes_rcvd = recvfrom(this->sockfd, buff, sizeof(buff), 0, (struct sockaddr *) &this->conn_info.client_addr, &this->conn_info.addr_size);
	if(bytes_rcvd == -1) {
		perror("recvfrom");
		exit(1);
	}

	return string(buff);
}

int Server::stop_and_wait(FILE* file) {
	cout << "Running Stop and Wait protocol..." << endl;
	string ack;

	bool write_done = false;

	int bytes_rcvd;
	int bytes_written;
	int bytes_sent;

	while(!write_done) {
		/* receive a packet */
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

		/* done */
		if(bytes_rcvd - this->conn_info.header_len < 0) {
			if(string(buffer).find("done") != string::npos) {
				write_done = true;
				cout << "Received 'done'" << endl;
				continue;
			}
		}

		/* parse out header */
		unsigned char client_cs[2];
		unsigned char header[this->conn_info.header_len - 2];
		unsigned char data[bytes_rcvd - this->conn_info.header_len];
		memcpy(client_cs, buffer, 2);
		memcpy(header, buffer + 2, this->conn_info.header_len - 2);
		memcpy(data, buffer + this->conn_info.header_len, bytes_rcvd - this->conn_info.header_len);

		string header_s(header, header + sizeof(header));

		string resent_s = header_s.substr(0, 1);
		string seq_num_s = header_s.substr(1, this->conn_info.header_len - 3);

		int resent = stoi(resent_s);
		ack = "ack" + seq_num_s;
		int seq_num = stoi(seq_num_s);

		cout << "Packet " << seq_num << " received" << endl;
		this->conn_info.packets_rcvd++;
		if(resent == 0) {
			this->conn_info.original_packets++;
		}

		if(verify_checksum(uchars_to_uint16(client_cs), create_checksum(data, sizeof(data)))) {
			cout << "Checksum OK" << endl;
			cout << "Ack " << seq_num << " sent" << endl;

			if(seq_num != this->conn_info.last_seq_num) {
				bytes_written = fwrite(data, 1, bytes_rcvd - this->conn_info.header_len, file);
				this->conn_info.total_bytes_written += bytes_written;
			}
			this->conn_info.last_seq_num = seq_num;

			/* lose acks */
			vector<int>::iterator position = find(this->conn_info.lost_acks.begin(), this->conn_info.lost_acks.end(), this->conn_info.packets_rcvd);
			if(position != this->conn_info.lost_acks.end()) {
				/* ack was "sent", but never got to client */
				cout << "Ack " << seq_num << " lost" << " (packets received: " << this->conn_info.packets_rcvd << ")" << endl;
				this->conn_info.lost_acks.erase(position);
			} else {
				/* send ack and write buffer to file */
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
			}
		} else {
			cout << "Checksum failed" << endl;
		}

		cout << endl;
	}

	return 0;
}

int Server::go_back_n(FILE* file) {
	cout << "Running Go-Back-N protocol..." << endl;

	string ack;
	bool write_done = false;
	int expected_seq_num = 0;
	int last_ack_num = -1;

	/* pad last_ack with correct amount of 0 */
	/* header_len - 3 is ack_len, - 2 for '-' and '1' */
	int padding = this->conn_info.header_len - 3 - 2;
	string last_ack = "ack-" + string(padding, '0') + "1";


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

		/* done */
		if(bytes_rcvd - this->conn_info.header_len < 0) {
			if(string(buffer).find("done") != string::npos) {
				write_done = true;
				cout << "Received 'done'" << endl;
				continue;
			}
		}

		/* parse out header */
		unsigned char client_cs[2];
		unsigned char header[this->conn_info.header_len - 2];
		unsigned char data[bytes_rcvd - this->conn_info.header_len];
		memcpy(client_cs, buffer, 2);
		memcpy(header, buffer + 2, this->conn_info.header_len - 2);
		memcpy(data, buffer + this->conn_info.header_len, bytes_rcvd - this->conn_info.header_len);

		string header_s(header, header + sizeof(header));

		string resent_s = header_s.substr(0, 1);
		string seq_num_s = header_s.substr(1, this->conn_info.header_len - 3);

		int resent = stoi(resent_s);
		ack = "ack" + seq_num_s;
		int seq_num = stoi(seq_num_s);

		cout << "Packet " << seq_num << " received" << endl;
		this->conn_info.packets_rcvd++;
		this->conn_info.last_seq_num = seq_num;
		if(resent == 0) {
			this->conn_info.original_packets++;
		}

		bool checksum = verify_checksum(uchars_to_uint16(client_cs), create_checksum(data, sizeof(data)));

		/* send ack and write buffer to file */
		if(checksum && seq_num == expected_seq_num) {
			cout << "Checksum OK" << endl;

			cout << "Ack " << seq_num << " sent" << endl;

			bytes_written = fwrite(data, 1, bytes_rcvd - this->conn_info.header_len, file);

			this->conn_info.total_bytes_written += bytes_written;
			expected_seq_num++;
			if(expected_seq_num > this->conn_info.max_seq_num) {
				expected_seq_num = 0;
			}
			last_ack = ack;
			last_ack_num = seq_num;

			/* lose acks */
			vector<int>::iterator position = find(this->conn_info.lost_acks.begin(), this->conn_info.lost_acks.end(), this->conn_info.packets_rcvd);
			if(position != this->conn_info.lost_acks.end()) {
				/* ack was "sent", but never got to client */
				cout << "Ack " << seq_num << " lost" << " (packets received: " << this->conn_info.packets_rcvd << ")" << endl;
				this->conn_info.lost_acks.erase(position);
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
			}
		} else {
			if(!checksum) {
				cout << "Checksum failed" << endl;
			}

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

			cout << "Ack " << last_ack_num << " sent" << endl;
		}

		cout << "Current window = [" << expected_seq_num << "]" << endl;
		cout << endl;
	}

	return 0;
}

int Server::selective_repeat(FILE* file) {
	cout << "Running Selective Repeat protocol..." << endl;

	Frame f = Frame();
	int recv_base = 0;
	int window_end = this->conn_info.window_size - 1;

	vector<Frame> window;

	bool write_done = false;
	while(!write_done) {
		char buffer[this->conn_info.packet_size + this->conn_info.header_len + 1];
		memset(buffer, 0, this->conn_info.packet_size + this->conn_info.header_len + 1);

		int bytes_rcvd = recvfrom(this->sockfd,
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

		/* done */
		if(bytes_rcvd - this->conn_info.header_len < 0) {
			if(string(buffer).find("done") != string::npos) {
				write_done = true;
				cout << "Received 'done'" << endl;
				continue;
			}
		}

		/* parse out header */
		unsigned char client_cs[2];
		unsigned char header[this->conn_info.header_len - 2];
		unsigned char data[bytes_rcvd - this->conn_info.header_len];
		memcpy(client_cs, buffer, 2);
		memcpy(header, buffer + 2, this->conn_info.header_len - 2);
		memcpy(data, buffer + this->conn_info.header_len, bytes_rcvd - this->conn_info.header_len);

		string header_s(header, header + sizeof(header));

		string resent_s = header_s.substr(0, 1);
		string seq_num_s = header_s.substr(1, this->conn_info.header_len - 3);

		int resent = stoi(resent_s);
		string ack = "ack" + seq_num_s;
		int seq_num = stoi(seq_num_s);

		cout << "Packet " << seq_num << " received" << endl;
		this->conn_info.last_seq_num = seq_num;
		this->conn_info.packets_rcvd++;
		if(resent == 0) {
			this->conn_info.original_packets++;
		}

		uint16_t server_cs = create_checksum(data, sizeof(data));
		bool checksum = verify_checksum(uchars_to_uint16(client_cs), server_cs);

		vector<unsigned char> f_data;
		for(int i = 0; i < this->conn_info.packet_size; i++) {
			f_data.push_back(data[i]);
		}

		f = Frame(seq_num, f_data, this->conn_info.header_len - 3, uint16bits_to_uchars(server_cs));

		bool in_window = false;
		if(recv_base + this->conn_info.window_size > this->conn_info.max_seq_num + 1) {
			/* sequence number wraparound in window */
			if((seq_num >= recv_base && seq_num <= this->conn_info.max_seq_num) || (seq_num >= 0 && seq_num <= window_end)) {
				in_window = true;
			}
		} else {
			if(seq_num >= recv_base && seq_num <= window_end) {
				in_window = true;
			}
		}

		/* packet not damaged and in current window */
		if(checksum && in_window) {
			cout << "Checksum OK" << endl;

			/* packet is smallest in window and can be written */
			if(seq_num == recv_base) {
				/* write */
				int bytes_written = fwrite(data, 1, bytes_rcvd - this->conn_info.header_len, file);
				this->conn_info.total_bytes_written += bytes_written;

				recv_base++; // recv_base = recv_base + 1 % max_seq_num + 1
				if(recv_base > this->conn_info.max_seq_num) {
					recv_base = 0;
				}
				window_end++;
				if(window_end > this->conn_info.max_seq_num) {
					window_end = 0;
				}

				/* loop through the current window and shift it until there are no more frames with seq_num = recv_base */
				size_t i = 0;
				size_t count = 0; /* number of frames that do not equal receive base */
				while(count < window.size()) {
					if(window[i].seq_num == recv_base) {
						/* write */
						bytes_written = fwrite(window[i].data.data(), 1, window[i].data.size(), file);
						this->conn_info.total_bytes_written += bytes_written;

						recv_base++;
						if(recv_base > this->conn_info.max_seq_num) {
							recv_base = 0;
						}
						window_end++;
						if(window_end > this->conn_info.max_seq_num) {
							window_end = 0;
						}

						window.erase(window.begin() + i);
						i--;
						count = 0;
					} else {
						count++;
					}

					i++;
					if(i >= window.size()) {
						i = 0;
					}
				}

			} else {
				window.push_back(f);
			}

			cout << "Ack " << seq_num << " sent" << endl;

			/* lose acks */
			vector<int>::iterator position = find(this->conn_info.lost_acks.begin(), this->conn_info.lost_acks.end(), this->conn_info.packets_rcvd);
			if(position != this->conn_info.lost_acks.end()) {
				/* ack was "sent", but never got to client */
				cout << "Ack " << seq_num << " lost" << " (packets received: " << this->conn_info.packets_rcvd << ")" << endl;
				this->conn_info.lost_acks.erase(position);
			} else {
				/* send ack */
				int bytes_sent = sendto(this->sockfd,
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
			}

		} else if(!checksum) {
			cout << "Checksum failed" << endl;
			string nak = "nak" + seq_num_s;
			/* send nak */
			cout << "Nak " << seq_num << " sent" << endl;

		} else {
			/* frame is left of the current window */
			/* reciever window will always be ahead of sender window, so the server would never get a packet ahead of the current window */
			/* send ack */
			int bytes_sent = sendto(this->sockfd,
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
		}

		if(!write_done) {
			/* print current window */
			cout << "Current window = [";
			for(size_t i = 0; i < window.size(); i++) {
				if(i == window.size() - 1) {
					cout << window[i].seq_num;
				} else {
					cout << window[i].seq_num << ", ";
				}
			}
			cout << "]" << endl;
		}
		cout << endl;
	}

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

	int file_number = 0;

	Server s = Server(argv[1], 10);
	s.start_server();

	while(true) {
		int handled = s.handle_connections(file_number);
		if(handled == 0) {
			file_number++;
		}

	}

	s.close_server();
	return 0;
}
