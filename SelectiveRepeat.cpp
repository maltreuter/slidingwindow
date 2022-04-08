#include "SelectiveRepeat.h"

using namespace std;

SelectiveRepeat::SelectiveRepeat(Client c) {
	this->client = c;
	this->total_bytes_read = 0;
	this->total_bytes_sent = 0;
	this->packets_sent = 0;
}

SelectiveRepeat::~SelectiveRepeat() {

}

int SelectiveRepeat::send() {
	/* error checking sendto */
	int bytes_sent;

	/* check if file exists */
	FILE *file = fopen(this->client.user.file_path.c_str(), "rb");

	vector<Frame> current_window;
	int send_base = 0;
	int next_seq_num = 0;

	bool read_done = false;
	Frame f = Frame();
	int start_time = this->client.get_current_time();
	int original_packets = 0;
	int resent_packets = 0;

	while(true) {
		/* window not full and frames to be sent */
		if(!read_done) {
			if(current_window.size() < (size_t) this->client.user.window_size) {
				f = this->client.getNextFrame(file, &read_done, next_seq_num);
				this->total_bytes_read += f.data.size();
				f.timer_running = true;
				f.timer_time = this->client.get_current_time();

				current_window.push_back(f);
				next_seq_num++;

				/* send current frame while checking for user specified errors */
				bytes_sent = this->client.send_frame_with_errors(f);

				if(bytes_sent == -1) {
					continue;
				}

				cout << "Packet " << f.seq_num << " sent" << endl;

				/* send_frame_with_errors returns -2 if packet was "lost" */
				if(bytes_sent == -2) {
					cout << "Packet " << f.seq_num << " lost" << endl;
					this->total_bytes_sent += this->client.user.header_len + f.data.size();
				} else {
					this->total_bytes_sent += bytes_sent;
				}

				/* we still "sent" the packet, it just got lost on the way */
				original_packets++;
				packets_sent++;
			}
		}

		bool nak = false;
		int ack_num = receive_ack(&nak);

		if(ack_num >= 0 && !nak) {
			cout << "Ack " << ack_num << " received" << endl;

			for(size_t i = 0; i < current_window.size(); i++) {
				if(current_window[i].seq_num == ack_num) {
					current_window[i].acked = true;
					current_window[i].timer_running = false;
					break;
				}
			}

			while(!current_window.empty()) {
				if(!current_window[0].acked) {
					break;
				}

				send_base++;
				current_window.erase(current_window.begin());
			}

			/* print current window */
			cout << "Current window = [";
			for(size_t i = 0; i < current_window.size(); i++) {
				if(i == current_window.size() - 1) {
					cout << current_window[i].seq_num;
				} else {
					cout << current_window[i].seq_num << ", ";
				}
			}
			cout << "]" << endl << endl;

		} else if (ack_num >= 0 && nak) {
			cout << "Nak " << ack_num << " received" << endl;

			/* print current window */
			cout << "Current window = [";
			for(size_t i = 0; i < current_window.size(); i++) {
				if(i == current_window.size() - 1) {
					cout << current_window[i].seq_num;
				} else {
					cout << current_window[i].seq_num << ", ";
				}
			}
			cout << "]" << endl << endl;

			Frame resend;
			for(size_t i = 0; i < current_window.size(); i++) {
				if(current_window[i].seq_num == ack_num) {
					current_window[i].timer_running = true;
					current_window[i].timer_time = this->client.get_current_time();
					resend = current_window[i];
					break;
				}
			}

			bytes_sent = this->client.send_frame(resend, true);

			if(bytes_sent == -1) {
				perror("sendto");
				continue;
			}

			cout << "Packet " << resend.seq_num << " retransmitted" << endl << endl;
			resent_packets++;
			packets_sent++;

			this->total_bytes_sent += bytes_sent;
		}

		/* timer */
		int current_time = this->client.get_current_time();
		for(size_t i = 0; i < current_window.size(); i++) {
			if(current_window[i].timer_running && current_time - current_window[i].timer_time > this->client.user.timeout_int) {
				cout << "Packet " << current_window[i].seq_num << " ***** Timed Out *****" << endl;

				current_window[i].timer_time = current_time;

				bytes_sent = this->client.send_frame(current_window[i], true);

				if(bytes_sent == -1) {
					continue;
				}

				cout << "Packet " << current_window[i].seq_num << " retransmitted" << endl << endl;
				resent_packets++;
				packets_sent++;

				this->total_bytes_sent += bytes_sent;
			}
		}


		if(read_done) {
			bool done = true;
			for(size_t i = 0; i < current_window.size(); i++) {
				if(!current_window[i].acked) {
					done = false;
					break;
				}
			}
			if(done) {
				break;
			}
		}
	}

	/* tell the server we are done sending frames */
	/* read_done set when we got EOF from fread */
	bytes_sent = this->client.send_to_server("done");
	if(bytes_sent == -1) {
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
	cout << "md5sum: " << get_md5(this->client.user.file_path) << endl;
	int time_m = this->client.get_current_time() - start_time;
	cout << "Elapsed time: " << time_m << " milliseconds (~" << time_m / 1000 << " seconds)" << endl;

	return 0;
}

int SelectiveRepeat::receive_ack(bool* nak) {
	struct pollfd fds[1];
	fds[0].fd = this->client.sockfd;
	fds[0].events = POLLIN;

	if(poll(fds, 1, 0) > 0) {
		/* something to receive */
		char ack[3 + this->client.user.header_len / 2];
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

		string ack_or_nak = string(ack).substr(0, 3);
		if(ack_or_nak.compare("nak") == 0) {
			*nak = true;
		}

		/* split seq_num */
		string ack_num = string(ack).substr(3, this->client.user.header_len / 2);

		return stoi(ack_num);
	}

	return -1;
}
