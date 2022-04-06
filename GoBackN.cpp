#include "GoBackN.h"

using namespace std;

GoBackN::GoBackN(Client c) {
	this->client = c;
	this->total_bytes_read = 0;
	this->total_bytes_sent = 0;
	this->packets_sent = 0;
}

GoBackN::~GoBackN() {

}

int GoBackN::send() {
	/* error checking sendto */
	int bytes_sent;

	/* check if file exists */
	FILE *file = fopen(this->client.user.file_path.c_str(), "rb");

	queue<Frame> current_window;
	int send_base = 0;
	int next_seq_num = 0;
	bool timer_running = false;
	int timer_time = 0;
	int last_frame_num = -2;

	bool read_done = false;
	Frame f = Frame();

	int original_packets = 0;
	int resent_packets = 0;
	int start_time = this->client.get_current_time();

	while(true) {
		/* window not full and frames to be sent */
		if(!read_done) {
			if(current_window.size() < (size_t) this->client.user.window_size) {
				f = this->client.getNextFrame(file, &read_done, next_seq_num);
				this->total_bytes_read += f.data.size();
				if(read_done) {
					last_frame_num = next_seq_num;
				}

				// string data_string(reinterpret_cast<char*>(f.data.data()));
				f.checksum = this->client.create_checksum(f.data.data(), f.data.size(), 8);
				cout << "checksum: " << f.checksum << endl;

				current_window.push(f);
				next_seq_num++;

				/* lose specific packets */
				vector<int>::iterator position = find(this->client.user.lost_packets.begin(), this->client.user.lost_packets.end(), f.seq_num);
				if(position == this->client.user.lost_packets.end()) {
					/* lost_packets does not contain f.seq_num */
					/* send current frame */

					f.checksum = this->client.create_checksum(f.data.data(), f.data.size(), 8);
					bytes_sent = this->client.send_frame(f);

					if(bytes_sent == -1) {
						continue;
					}

					cout << "Packet " << f.seq_num << " sent" << endl;

					this->total_bytes_sent += bytes_sent;
				} else {
					/* lose this packet  */
					cout << "Packet " << f.seq_num << " lost" << endl;
					this->client.user.lost_packets.erase(position);
				}

				/* we still "sent" the packet, it just got lost on the way */
				original_packets++;
				this->packets_sent++;
			}
		}

		int ack_num = receive_ack();

		/* if we've received an ack for the last frame: done */
		if(ack_num == last_frame_num) {
			break;
		}

		/* ack received */
		if(ack_num >= 0) {
			/* shift window */
			if(ack_num >= send_base && ack_num < next_seq_num) {
				while(send_base <= ack_num) {
					send_base++;
					current_window.pop();
				}
				timer_running = false;
			}

			/* print current window */
			cout << "Current window = [";
			queue<Frame> tmp = current_window;
			while(!tmp.empty()) {
				Frame frame = tmp.front();
				if(tmp.size() == 1) {
					cout << frame.seq_num;
					tmp.pop();
				} else {
					cout << frame.seq_num << ", ";
					tmp.pop();
				}
			}
			cout << "]" << endl;

		} else {
			if(!timer_running) {
				timer_running = true;
				timer_time = this->client.get_current_time();
			}
		}

		/* if timer == timeout */
		if(timer_running && this->client.get_current_time() - timer_time > this->client.user.timeout_int) {
			cout << "***** Timed Out *****" << endl;
			queue<Frame> tmp = current_window;
			while(!tmp.empty()) {
				Frame resend = tmp.front();
				tmp.pop();

				bytes_sent = this->client.send_frame(resend);

				if(bytes_sent == -1) {
					continue;
				}

				cout << "Packet " << resend.seq_num << " retransmitted" << endl;
				resent_packets++;
				this->packets_sent++;

				this->total_bytes_sent += bytes_sent;

			}
			timer_time = this->client.get_current_time();
		}

	}

	/* tell the server we are done sending frames */
	/* read_done set when we got EOF from fread */
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

	/* print stats */
	cout << "\n************************************" << endl;
	cout << "Sent file: " << this->client.user.file_path << endl;
	cout << "Number of original packets sent: " << original_packets << endl;
	cout << "Number of retransmitted packets: " << resent_packets << endl;
	cout << "Total number of packets sent: " << this->packets_sent << endl;
	cout << "Total bytes read from file: " << this->total_bytes_read << endl;
	cout << "Total bytes sent to server: " << this->total_bytes_sent << endl;
	int time_m = this->client.get_current_time() - start_time;
	cout << "Elapsed time: " << time_m << " milliseconds (~" << time_m / 1000 << " seconds)" << endl;

	return 0;
}

int GoBackN::receive_ack() {
	struct pollfd fds[1];
	fds[0].fd = this->client.sockfd;
	fds[0].events = POLLIN;

	if(poll(fds, 1, 0) > 0) {
		/* soemthing to receive */
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
		}

		/* split seq_num */
		string ack_num = string(ack).substr(3, this->client.user.header_len / 2);
		cout << "ack " << ack_num << " received" << endl;

		return stoi(ack_num);
	}

	return -1;
}
