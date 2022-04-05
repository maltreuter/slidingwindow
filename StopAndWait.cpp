#include "StopAndWait.h"

using namespace std;

StopAndWait::StopAndWait(Client c) {
	this->client = c;
	this->total_bytes_read = 0;
	this->total_bytes_sent = 0;
	this->packets_sent = 0;
}

StopAndWait::~StopAndWait() {

}

int StopAndWait::send() {
	/* error checking sendto */
	int bytes_sent;

	/* check if file exists */
	FILE *file = fopen(this->client.user.file_path.c_str(), "rb");

	vector<Frame> frames;

	bool read_done = false;
	bool resend = false;

	/* to store previous frame for resending if necessary */
	Frame f = Frame();
	int last_frame_num = -2;

	while(true) {
		if(!resend) {
			f = this->client.getNextFrame(file, &read_done, this->packets_sent);
			if(read_done) {
				last_frame_num = f.seq_num;
			}
		}

		int send_time = this->client.get_current_time();

		vector<int>::iterator position = find(this->client.user.lost_packets.begin(), this->client.user.lost_packets.end(), f.seq_num);
		if(position == this->client.user.lost_packets.end()){
			/* lost_packets does not contain f.seq_num */
			/* send current frame */
			string data_string(reinterpret_cast<char*>(f.data.data()));
			cout << endl;
			cout << "data length: " << data_string.length() << endl;
			f.checksum = this->client.create_checksum(data_string, 8);
			cout << "checksum: " << f.checksum << endl;
			bytes_sent = this->client.send_frame(f);

			if(bytes_sent == -1) {
				resend = true;
				continue;
			}

			this->total_bytes_sent += bytes_sent;

			if(resend) {
				cout << "resent packet " << f.seq_num << endl;
			} else {
				cout << "sent packet " << f.seq_num << endl;
			}
		} else {
			/* lose this packet and remove it from lost_packets */
			cout << "Packet " << f.seq_num << " lost." << endl;
			this->client.user.lost_packets.erase(position);
		}

		int ack_num = receive_ack(send_time);
		if(ack_num == -1) {
			resend = true;
		} else {
			resend = false;
		}

		if(!resend) {
			frames.push_back(f);
			this->total_bytes_read += f.data.size();
		}

		/* done reading and last frame has been acked */
		if(read_done && ack_num == last_frame_num) {
			break;
		}
	}

	/* tell the server we are done sending frames */
	/* read_done set when we got EOF from fread */
	if(read_done) {
		bytes_sent = sendto(this->client.sockfd,
			"done",
			4,
			0,
			this->client.server_addr,
			this->client.server_addr_len
		);
		if(bytes_sent == -1) {
			perror("sendto");
			exit(1);
		}
		this->total_bytes_sent += bytes_sent;
	}

	/* print stats */
	cout << "\n************************************" << endl;
	cout << "Sending file: " << this->client.user.file_path << endl;
	// cout << "md5 sum: " << get_md5(this->client.user.file_path) << endl;
	cout << "packets sent: " << this->packets_sent << endl;
	cout << "total bytes read from file: " << this->total_bytes_read << endl;
	cout << "total bytes sent to server: " << this->total_bytes_sent << endl;

	return 0;
}

int StopAndWait::receive_ack(int send_time) {
	struct pollfd fds[1];
	fds[0].fd = this->client.sockfd;
	fds[0].events = POLLIN;

	/* Receive Ack */
	while(true) {
		int now = this->client.get_current_time();
		if(now - send_time > this->client.user.timeout_int) {
			cout << "Timeout: " << now - send_time << endl;
			cout << "Packet " << this->packets_sent << " timed out" << endl;
			return -1;
		} else if(poll(fds, 1, 0) > 0) {
			/* receive ack */
			/* "ack0001\0" "ack9945\0" */
			char ack[3 + this->client.user.header_len];
			int bytes_rcvd = recvfrom(this->client.sockfd,
				ack,
				sizeof(ack),
				0,
				this->client.server_addr,
				&this->client.server_addr_len
			);
			if(bytes_rcvd == -1) {
				perror("recvfrom");
				continue;
			}

			/* split seq_num */
			string ack_num = string(ack).substr(3, this->client.user.header_len / 2);
			cout << "ack " << ack_num << " received" << endl;

			this->packets_sent++;

			return stoi(ack_num);
		}
	}
}
