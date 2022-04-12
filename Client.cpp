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
		-1, /* ack len */
		1 + 2, /* header length (1 for resent, 2 for checksum, ack_len)*/
		vector<int>(), /* lost packets */
		vector<int>(), /* lost_acks */
		vector<int>() /*corrupt_packets */

	};
	
	this->address_info = NULL;
	this->sockfd = -1;
}

Client::~Client() {

}

int Client::connect() {
	struct addrinfo hints, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfo(this->user.host.c_str(), this->user.port.c_str(), &hints, &this->address_info);

	for(p = this->address_info; p != NULL; p = p->ai_next) {
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
	freeaddrinfo(this->address_info);
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

	vector<unsigned char> checksum = uint16bits_to_uchars(~create_checksum(data.data(), data.size()));
	Frame f = Frame(seq_num, data, this->user.ack_len, checksum);
	// f.checksum = create_checksum(f.data.data(), f.data.size(), 8);
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

	header += f.padSeqNum();

	memcpy(curr_frame, f.checksum.data(), 2);
	memcpy(curr_frame + 2, header.c_str(), header.length());
	memcpy(curr_frame + 2 + header.length(), f.data.data(), f.data.size());

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

	if(resend) {
		cout << "Packet " << f.seq_num << " retransmitted" << endl;
	} else {
		cout << "Packet " << f.seq_num << " sent" << endl;
	}

	return bytes_sent;
}

int Client::send_frame_with_errors(Frame f, bool resend, int packets_sent) {
	if(this->user.errors == 1) {
		/* check if packet should be lost */
		vector<int>::iterator position = find(this->user.corrupt_packets.begin(), this->user.corrupt_packets.end(), packets_sent + 1);

		/* if packet should be corrupted */
		if(position != this->user.corrupt_packets.end()) {
			/* corrupt packet data */
			swap(f.data[0], f.data[f.data.size() - 1]);
			cout << "Corrupted packet " << f.seq_num << " (packets sent: " << packets_sent + 1 << ")" <<  endl;
			this->user.corrupt_packets.erase(position);
		}

		position = find(this->user.lost_packets.begin(), this->user.lost_packets.end(), packets_sent + 1);

		/* if packet should not be lost */
		if(position == this->user.lost_packets.end()) {
			return send_frame(f, resend);
		} else {
			if(resend) {
				cout << "Packet " << f.seq_num << " retransmitted" << endl;
			} else {
				cout << "Packet " << f.seq_num << " sent" << endl;
			}
			cout << "Packet " << f.seq_num << " lost" << " (packets sent: " << packets_sent + 1 << ")" << endl;
			this->user.lost_packets.erase(position);
			return this->user.header_len + f.data.size();
		}
	} else {
		return send_frame(f, resend);
	}
}
