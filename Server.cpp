#include "Server.h"

using namespace std;

bool sort_frame(Frame a, Frame b) {
	return a.seq_num < b.seq_num;
}

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

		if(this->conn_info.protocol == 0) {
			stop_and_wait(file);
		} else if(this->conn_info.protocol == 1) {
			go_back_n(file);
		} else if(this->conn_info.protocol == 2) {
			selective_repeat(file);
		}

		fclose(file);

		// string out_md5 = get_md5(filesystem::path(file_path));

		cout << "\n************************************" << endl;
		cout << "Wrote to file: '" << file_path << endl;
		cout << "Last packet seq# received: " << this->conn_info.last_seq_num << endl;
		cout << "Number of original packets received: " << this->conn_info.original_packets << endl;
		cout << "Number of retransmitted packets received: " << this->conn_info.packets_rcvd - this->conn_info.original_packets << endl;
		cout << "Number of packets received: " << this->conn_info.packets_rcvd << endl;
		cout << "Number of bytes written: " << this->conn_info.total_bytes_written << endl;
		cout << "md5sum: " << get_md5(file_path) << endl;

		cout << "Client disconnected" << endl;
		loop++;
	}
}

int Server::handshake() {
	struct sockaddr_storage client_addr = {0};
	this->conn_info = {
		client_addr,
		sizeof(client_addr),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		-1
	};

	this->conn_info.lost_acks = vector<int>();
	this->conn_info.lost_acks.push_back(20);
	this->conn_info.lost_acks.push_back(50);

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

	/* receive protocol */
	char protocol[8];
	bytes_rcvd = recvfrom(this->sockfd, protocol, 7, 0, (struct sockaddr *) &this->conn_info.client_addr, &this->conn_info.addr_size);
	if(bytes_rcvd == -1) {
		perror("recvfrom");
		exit(1);
	}
	protocol[bytes_rcvd] = '\0';

	this->conn_info.protocol = atoi(protocol);

	/* Receive errors */

	/* Receive window size */
	char window_size[8];
	bytes_rcvd = recvfrom(this->sockfd, window_size, 7, 0, (struct sockaddr *) &this->conn_info.client_addr, &this->conn_info.addr_size);
	if(bytes_rcvd == -1) {
		perror("recvfrom");
		exit(1);
	}
	window_size[bytes_rcvd] = '\0';

	this->conn_info.window_size = atoi(window_size);

	return 0;
}

int Server::stop_and_wait(FILE* file) {
	cout << "Running Stop and Wait protocol..." << endl;
	/* open file to write */
	string ack;

	bool write_done = false;

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

		cout << "bytes_rcvd: " << bytes_rcvd << endl;

		/* done */
		if(bytes_rcvd - this->conn_info.header_len < 0) {
			if(string(buffer).find("done") != string::npos) {
				write_done = true;
				cout << "Received 'done'" << endl;
				continue;
			}
		}

		/* parse out header */
		unsigned char header[this->conn_info.header_len];
		unsigned char data[bytes_rcvd - this->conn_info.header_len];
		memcpy(header, buffer, this->conn_info.header_len);
		memcpy(data, buffer + this->conn_info.header_len, bytes_rcvd - this->conn_info.header_len);

		// cout << "size of data: " << sizeof(data) << endl;

		string header_s(header, header + sizeof(header));

		string checksum_s = header_s.substr(0, this->conn_info.header_len / 2);
		string seq_num_s = header_s.substr(this->conn_info.header_len / 2, this->conn_info.header_len / 2);

		/* client will send "done" when it is finished sending the file */
		/* look for "done" in the buffer to stop looping, otherwise write to file */
		if(check_checksum(checksum_s, data, sizeof(data), 8)) {
			/* send ack and write buffer to file */
			ack = "ack" + seq_num_s;
			int seq_num = stoi(seq_num_s);

			cout << "Packet " << seq_num << " received" << endl;
			cout << "Checksum OK" << endl;
			this->conn_info.last_seq_num = seq_num;

			// check if we should lose any acks
			vector<int>::iterator position = find(this->conn_info.lost_acks.begin(), this->conn_info.lost_acks.end(), seq_num);
			if(position != this->conn_info.lost_acks.end()) {
				cout << "Ack " << seq_num << " lost" << endl;
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

				cout << "Ack " << seq_num << " sent" << endl;

				bytes_written = fwrite(data, 1, bytes_rcvd - sizeof(header), file);

				this->conn_info.total_bytes_written += bytes_written;
				this->conn_info.packets_rcvd++;
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
	string last_ack = "";
	int last_ack_num;

	int bytes_rcvd;
	int bytes_written;
	int bytes_sent;;

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

		/* if bytes_rcvd < header_len == bad tings */

		/* parse out header */
		unsigned char header[this->conn_info.header_len];
		unsigned char data[bytes_rcvd - this->conn_info.header_len];
		memcpy(header, buffer, this->conn_info.header_len);
		memcpy(data, buffer + this->conn_info.header_len, bytes_rcvd - this->conn_info.header_len);

		string header_s(header, header + sizeof(header));

		string checksum_s = header_s.substr(0, this->conn_info.header_len / 2);
		string seq_num_s = header_s.substr(this->conn_info.header_len / 2, this->conn_info.header_len / 2);

		/* client will send "done" when it is finished sending the file */
		/* look for "done" in the buffer to stop looping, otherwise write to file */
		/* done */
		if(bytes_rcvd - this->conn_info.header_len < 0) {
			if(string(buffer).find("done") != string::npos) {
				write_done = true;
				cout << "Received 'done'" << endl;
				continue;
			}
		} else {
			ack = "ack" + seq_num_s;
			int seq_num = stoi(seq_num_s);

			cout << "Packet " << seq_num << " received" << endl;
			this->conn_info.packets_rcvd++;
			this->conn_info.last_seq_num = seq_num;

			bool checksum = check_checksum(checksum_s, data, sizeof(data), 8);

			vector<int>::iterator position = find(this->conn_info.lost_acks.begin(), this->conn_info.lost_acks.end(), seq_num);
			if(checksum && position != this->conn_info.lost_acks.end()) {
				cout << "Checksum OK" << endl;
				cout << "Ack " << seq_num << " lost" << endl;
				this->conn_info.lost_acks.erase(position);
			} else {
				/* send ack and write buffer to file */
				if(checksum && seq_num == expected_seq_num) {
					cout << "Checksum OK" << endl;

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

					cout << "Ack " << seq_num << " sent" << endl;

					bytes_written = fwrite(data, 1, bytes_rcvd - sizeof(header), file);

					this->conn_info.total_bytes_written += bytes_written;
					this->conn_info.original_packets++;
					expected_seq_num++;
					last_ack = ack;
					last_ack_num = seq_num;

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
			}
			cout << "Current window = [" << expected_seq_num << "]" << endl;
		}

	}

	return 0;
}

int Server::selective_repeat(FILE* file) {
	cout << "Running Selective Repeat protocol..." << endl;

	Frame f = Frame();
	int recv_base = 0;
	// bool sent_neg_ack = false;

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

		/* if bytes_rcvd < header_len == bad tings */

		/* parse out header */
		unsigned char header[this->conn_info.header_len];
		unsigned char data[bytes_rcvd - this->conn_info.header_len];
		memcpy(header, buffer, this->conn_info.header_len);
		memcpy(data, buffer + this->conn_info.header_len, bytes_rcvd - this->conn_info.header_len);

		string header_s(header, header + sizeof(header));

		string checksum_s = header_s.substr(0, this->conn_info.header_len / 2);
		string seq_num_s = header_s.substr(this->conn_info.header_len / 2, this->conn_info.header_len / 2);

		string ack;

		/* done */
		if(bytes_rcvd - this->conn_info.header_len < 0) {
			if(string(buffer).find("done") != string::npos) {
				write_done = true;
				cout << "Received 'done'" << endl;
				continue;
			}
		} else {
			ack = "ack" + seq_num_s;
			int seq_num = stoi(seq_num_s);

			cout << "Packet " << seq_num << " received" << endl;
			this->conn_info.last_seq_num = seq_num;
			this->conn_info.packets_rcvd++;

			bool checksum = check_checksum(checksum_s, data, sizeof(data), 8);

			vector<unsigned char> f_data;
			for(int i = 0; i < this->conn_info.packet_size; i++) {
				f_data.push_back(data[i]);
			}

			f = Frame(seq_num, f_data, this->conn_info.header_len);


			/* packet not damaged and in current window */
			if(checksum && (f.seq_num >= recv_base) && (f.seq_num < (recv_base + this->conn_info.window_size))) {
				cout << "Checksum OK" << endl;

				/* lose acks */
				vector<int>::iterator position = find(this->conn_info.lost_acks.begin(), this->conn_info.lost_acks.end(), seq_num);
				if(position != this->conn_info.lost_acks.end()) {
					cout << "Ack " << seq_num << " lost" << endl;
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

					cout << "Ack " << seq_num << " sent" << endl;
					this->conn_info.original_packets++;

					/* packet is smallest in window and can be written */
					if(f.seq_num == recv_base) {
						/* write */
						int bytes_written = fwrite(data, 1, bytes_rcvd - sizeof(header), file);
						this->conn_info.total_bytes_written += bytes_written;

						recv_base++; // recv_base = recv_base + 1 % max_seq_num + 1
						/* check other out of order frames */
						for(size_t i = 0; i < window.size(); i++) {
							if(window[i].seq_num == recv_base) {
								/* write */
								bytes_written = fwrite(window[i].data.data(), 1, window[i].data.size(), file);
								this->conn_info.total_bytes_written += bytes_written;

								recv_base++;
								window.erase(window.begin() + i);
								i--;
							}
						}
					} else {
						window.push_back(f);
						sort(window.begin(), window.end(), sort_frame);
					}
				}

				/* if checksum == false, packet is corrupted */
			} else if(!checksum) {
				cout << "Checksum failed" << endl;;
				string nak = "nak" + seq_num_s;
				/* send nak */
				cout << "Nak " << seq_num << " sent" << endl;;
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
		}
	}

	return 0;
}

bool Server::check_checksum(string checksum, unsigned char *data, int dataLength, int blockSize) {
	// cout << "dataLength: " << dataLength << endl;
	int paddingSize = 0;
	unsigned char pad = '0';
	if (dataLength % blockSize != 0) {
        paddingSize = blockSize - (dataLength % blockSize);
    }
	unsigned char padding[paddingSize];
	for (int i = 0; i < paddingSize; i++) {
		padding[i] = pad;
	}

	unsigned char buffer[dataLength + paddingSize];
	memcpy(buffer, padding, paddingSize);
	memcpy(buffer + paddingSize, data, dataLength);

	int currentSum = 0;
    int currentCarry = 0;
    string recvSum = "";

    string binaryString = "";

    string newBinary = "";
    for (int i = 0; i < blockSize; i++) {
        newBinary += uchar_to_binary(buffer[i]);
    }

    for (int i = blockSize; i < dataLength; i = i + blockSize) {
        string nextBlock = "";
        for (int j = i; j < i + blockSize; j++) {
            nextBlock += uchar_to_binary(buffer[j]);
        }

        string binaryAddition = "";
        int currentSum = 0;
        int currentCarry = 0;

        for (int n = blockSize - 1; n >= 0; n--) {
            currentSum = currentSum + (nextBlock[n] - '0') + (newBinary[n] - '0');
            currentCarry = currentSum / 2;
            if (currentSum == 0 || currentSum == 2) {
                binaryAddition = '0' + binaryAddition;
                currentSum = currentCarry;
            } else {
                binaryAddition = '1' + binaryAddition;
                currentSum = currentCarry;
            }
        }

        string finalAddition = "";
        if (currentCarry == 1) {
            for (int k = binaryAddition.length() - 1; k >= 0; k--) {
                if (currentCarry == 0) {
                    finalAddition = binaryAddition[k] + finalAddition;
                } else if (((binaryAddition[k] - '0') + currentCarry) % 2 == 0) {
                    finalAddition = "0" + finalAddition;
                    currentCarry = 1;
                } else {
                    finalAddition = "1" + finalAddition;
                    currentCarry = 0;
                }
            }
            binaryString = finalAddition;
        } else {
            binaryString = binaryAddition;
        }
    }

    // cout << "checksum:   " << checksum << endl;
    // cout << "recvstring: " << binaryString << endl;

    for (int n = blockSize - 1; n >= 0; n--) {
        currentSum = currentSum + (checksum[n] - '0') + (binaryString[n] - '0');
        currentCarry = currentSum / 2;
        if (currentSum == 0 || currentSum == 2) {
            recvSum = '0' + recvSum;
            currentSum = currentCarry;
        } else {
            recvSum = '1' + recvSum;
            currentSum = currentCarry;
        }
    }

	// cout << "result: " << recvSum << endl;

	if (count(recvSum.begin(), recvSum.end(), '1') == blockSize) {
        return true;
    } else {
        return false;
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

	/* signal handler for ctrl-c */
	s.close_server();
}
