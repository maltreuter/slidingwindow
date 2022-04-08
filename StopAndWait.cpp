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

	bool read_done = false;
	bool resend = false;

	int original_packets = 0;
	int resent_packets = 0;

	/* to store previous frame for resending if necessary */
	Frame f = Frame();
	int last_frame_num = -2;

	int start_time = this->client.get_current_time();

	while(true) {
		if(!resend) {
			/* read next frame from file */
			f = this->client.getNextFrame(file, &read_done, this->packets_sent);
			if(read_done) {
				last_frame_num = f.seq_num;
			}
		}

		int send_time = this->client.get_current_time();

		/* send current frame */
		bytes_sent = this->client.send_frame(f);

		if(bytes_sent == -1) {
			/* some error sending, resend the frame */
			resend = true;
			continue;
		}

		if(resend) {
			cout << "Packet " << f.seq_num << " retransmitted" << endl;
			resent_packets++;
		} else {
			cout << "Packet " << f.seq_num << " sent" << endl;
			original_packets++;
		}

		/* send_frame returns -2 if packet was "lost" */
		if(bytes_sent == -2) {
			/* we still "sent" the packet, it just got lost on the way */
			cout << "Packet " << f.seq_num << " lost" << endl;
			this->total_bytes_sent += this->client.user.header_len + f.data.size();
		} else {
			this->total_bytes_sent += bytes_sent;
		}

		int ack_num = receive_ack(send_time);
		if(ack_num == -1) {
			resend = true;
		} else {
			resend = false;
		}

		if(!resend) {
			this->total_bytes_read += f.data.size();
		}

		/* done reading and last frame has been acked */
		if(read_done && ack_num == last_frame_num) {
			break;
		}

		cout << endl;
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
	cout << "Sent file: " << this->client.user.file_path << endl;
	cout << "Number of original packets sent: " << original_packets << endl;
	cout << "Number of retransmitted packets: " << resent_packets << endl;
	cout << "Total number of packets sent: " << this->packets_sent << endl;
	cout << "Total bytes read from file: " << this->total_bytes_read << endl;
	cout << "Total bytes sent to server: " << this->total_bytes_sent << endl;
	cout << "md5sum: " << get_md5(this->client.user.file_path) << endl;
	int time_m = this->client.get_current_time() - start_time;
	cout << "Elapsed time: " << time_m << " milliseconds (~" << time_m / 1000 << " seconds)" << endl;

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
			cout << "Packet " << this->packets_sent << " ***** Timed Out *****" << endl;
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
			string ack_num_s = string(ack).substr(3, this->client.user.header_len / 2);
			int ack_num = stoi(ack_num_s);
			cout << "Ack " << ack_num << " received" << endl;

			this->packets_sent++;

			return ack_num;
		}
	}
}
