#include "Client.h"

using namespace std;

Client::Client() {
	this->user = {
		"", /* file path */
		"", /* host */
		"", /* port */
		-1, /* packet_size */
		-1, /* timeout interval */
		-1, /* window size */
		-1, /* max sequence number */
		-1, /* situational errors */
		-1, /* protocol */
		17, /* header length (1 for resent, 8 for seq num, 8 for checksum)*/
		vector<int>(), /* lost packets */
		vector<int>(), /* lost_acks */
		vector<int>() /*corrupt_packets */
		
	};

	this->sockfd = -1;
}

Client::~Client() {

}

int Client::connect() {
	struct addrinfo hints, *address_info, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfo(this->user.host.c_str(), this->user.port.c_str(), &hints, &address_info);

	for(p = address_info; p != NULL; p = p->ai_next) {
		this->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(this->sockfd == -1) {
			perror("socket");
			continue;
		}

		cout << "Connected" << endl;
		this->server_addr = p->ai_addr;
		this->server_addr_len = p->ai_addrlen;
		return 0;
	}
	return -1;
}

int Client::disconnect() {
	close(this->sockfd);
	return 0;
}

int Client::handshake() {
	/* send packet size to server */
	// cout << this->user.packet_size << endl;
	// cout << this->user.header_len << endl;
	//

	if(send_to_server(to_string(this->user.packet_size)) == -1) {
		return -1;
	}
	
	/* send header len to server */
	if(send_to_server(to_string(this->user.header_len)) == -1) {
		return -1;
	}
	
	/* send protocol to server */
	if(send_to_server(to_string(this->user.protocol)) == -1) {
		return -1;
	}

	/* send errors */
	if(send_to_server(to_string(this->user.errors)) == -1) {
		return -1;
	}

	/* send acks to lose */
	if(this->user.errors == 1) {
		if(send_to_server(vector_to_string(this->user.lost_acks)) == -1) {
			return -1;
		}
	}

	/* send window size */
	if(send_to_server(to_string(this->user.window_size)) == -1) {
		return -1;
	}

	/*send max sequence number */
	if(send_to_server(to_string(this->user.max_seq_num)) == -1) {
		return -1;
	}
	
	return 0;
}

int Client::send_to_server(string send_str) {
	int bytes_sent = sendto(this->sockfd,
	send_str.c_str(),
	send_str.length(),
	0,
	this->server_addr,
	this->server_addr_len
	);

	if(bytes_sent == -1) {
		perror("sendto");
		return bytes_sent;
	}

	return bytes_sent;
}

Frame Client::getNextFrame(FILE* file, bool* read_done, int seq_num) {
	unsigned char frame_data[this->user.packet_size];
	memset(frame_data, 0, this->user.packet_size);

	/* read a frame from the file */
	/* fread returns packet_size on success */
	/* fread returns less than packet_size if EOF or error */
	int bytes_read = fread(frame_data, 1, this->user.packet_size, file);
	if(bytes_read < this->user.packet_size && this->user.packet_size != 0) {
		if(ferror(file) != 0) {		/* error */
			perror("fread");
			exit(1);
		}
		if(feof(file) != 0) {	/* end of file = true */
			cout << "end of file caught" << endl << endl;
			fclose(file);
			*read_done = true;
		}
	}

	vector<unsigned char> data;
	for(int i = 0; i < bytes_read; i++) {
		data.push_back(frame_data[i]);
	}

	Frame f = Frame(seq_num, data, this->user.header_len);
	f.checksum = create_checksum(f.data.data(), f.data.size(), 8);
	return f;
}

int Client::send_frame(Frame f, bool resend) {
	/* package frame for delivery */
	int buffer_size = this->user.header_len + f.data.size();
	unsigned char curr_frame[buffer_size];

	string header;
	if(resend) {
		header = "1";
	} else {
		header = "0";
	}

	header += f.checksum + f.padSeqNum();

	memcpy(curr_frame, header.c_str(), header.length());
	memcpy(curr_frame + header.length(), f.data.data(), f.data.size());

	/* send frame */
	int bytes_sent = sendto(this->sockfd,
		curr_frame,
		this->user.header_len + f.data.size(),
		0,
		this->server_addr,
		this->server_addr_len
	);

	if(bytes_sent == -1) {
		perror("sendto");
	}

	return bytes_sent;
}

int Client::send_frame_with_errors(Frame f, int packet_num) {
	/* check if packet should be lost */
	vector<int>::iterator position = find(this->user.lost_packets.begin(), this->user.lost_packets.end(), packet_num);

	/* if packet should not be lost */
	if(position == this->user.lost_packets.end()) {

		/* check if packet should be corrupted */
		position = find(this->user.corrupt_packets.begin(), this->user.corrupt_packets.end(), packet_num);

		/* if packet should not be corrupt */
		if(position == this->user.corrupt_packets.end()) {
			/* send it */
			return send_frame(f, false);
		} else {
			/* corrupt packet data then send it */
			swap(f.data[0], f.data[f.data.size() - 1]);
			cout << "Corrupted packet " << f.seq_num << " (packet number " << packet_num << ")" <<  endl;
			return send_frame(f, false);
		}

	} else {
		return -2;
	}
}

string Client::create_checksum(unsigned char *data, int dataLength, int blockSize) {
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
    string checksum = binaryString;
    for (size_t i = 0; i < binaryString.length(); i++) {
        if (binaryString[i] == '0') {
            checksum[i] = '1';
        } else {
            checksum[i] = '0';
        }
    }
    return checksum;
}
