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
		-1, /* situational errors */
		-1, /* protocol */
		16, /* header length */
		vector<int>() /* lost packets */
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

	int bytes_sent = sendto(this->sockfd,
		to_string(this->user.packet_size).c_str(),
		to_string(this->user.packet_size).length(),
		0,
		this->server_addr,
		this->server_addr_len
	);
	if(bytes_sent == -1) {
		perror("sendto");
		return -1;
	}

	/* send header len to server */
	bytes_sent = sendto(this->sockfd,
		to_string(this->user.header_len).c_str(),
		to_string(this->user.header_len).length(),
		0,
		this->server_addr,
		this->server_addr_len
	);
	if(bytes_sent == -1) {
		perror("sendto");
		return -1;
	}

	/* send protocol to server */
	bytes_sent = sendto(this->sockfd,
	to_string(this->user.protocol).c_str(),
	to_string(this->user.protocol).length(),
	0,
	this->server_addr,
	this->server_addr_len
	);
	if(bytes_sent == -1) {
		perror("sendto");
		return -1;
	}

	/* send errors */

	/* send window size */
	bytes_sent = sendto(this->sockfd,
	to_string(this->user.window_size).c_str(),
	to_string(this->user.window_size).length(),
	0,
	this->server_addr,
	this->server_addr_len
	);
	if(bytes_sent == -1) {
		perror("sendto");
		return -1;
	}

	return 0;
}

int Client::get_current_time() {
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

Frame Client::getNextFrame(FILE* file, bool* read_done, int packets_sent) {
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
			cout << "end of file caught" << endl;
			fclose(file);
			*read_done = true;
		}
	}

	vector<unsigned char> data;
	for(int i = 0; i < bytes_read; i++) {
		data.push_back(frame_data[i]);
	}
	return Frame(packets_sent, data, this->user.header_len);
}

int Client::send_frame(Frame f) {
	/* package frame for delivery */
	int buffer_size = this->user.header_len + f.data.size();
	unsigned char curr_frame[buffer_size];

	string header = f.checksum + f.padSeqNum();
	cout << "header: " << header << endl;
	cout << "header size: " << header.length() << endl;

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

string Client::create_checksum(string data, int blockSize) {
	int dataLength = data.length();
    if (dataLength % blockSize != 0) {
        int paddingSize = blockSize - (dataLength % blockSize);
        for (int i = 0; i < paddingSize; i++) {
            data = '0' + data;
        }
    }

    string binaryString = "";
    for (int i = 0; i < blockSize; i++) {
        binaryString = binaryString + data[i];
    }
    string newBinary = "";
    for (char &_char : binaryString) {
        newBinary += bitset<8>(_char).to_string();
    }
    //cout << "\nNewBinary: " << newBinary;

    for (int i = blockSize; i < dataLength; i = i + blockSize) {
        string next = "";
        for (int j = i; j < i + blockSize; j++) {
            next = next + data[j];
        }
        string nextBlock = "";
        for (char &_char : next) {
            nextBlock += bitset<8>(_char).to_string();
        }

       //cout << "\nnextBlock: " << nextBlock;

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
    for (int i = 0; i < binaryString.length(); i++) {
        if (binaryString[i] == '0') {
            checksum[i] = '1';
        } else {
            checksum[i] = '0';
        }
    }
    return checksum;
}
