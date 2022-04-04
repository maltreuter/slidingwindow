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

	while(true) {
		/* window not full and frames to be sent */
		if(!read_done) {
			if(current_window.size() < (size_t) this->client.user.window_size) {
				f = this->client.getNextFrame(file, &read_done, next_seq_num);
				f.timer_running = true;
				f.timer_time = this->client.get_current_time();
				current_window.push_back(f);
				next_seq_num++;

				bytes_sent = this->client.send_frame(f);

				if(bytes_sent == -1) {
					continue;
				}

				cout << "sent packet " << f.seq_num << endl;

				this->total_bytes_sent += bytes_sent;
			}
		}

		bool nak = false;
		int ack_num = receive_ack(&nak);
		if(ack_num >= 0 && !nak) {
			for(int i = 0; i < current_window.size(); i++) {
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

		} else if (ack_num >= 0 && nak) {
			Frame resend;
			for(int i = 0; i < current_window.size(); i++) {
				if(current_window[i].seq_num == ack_num) {
					current_window[i].timer_running = true;
					current_window[i].timer_time = this->client.get_current_time();
					resend = current_window[i];
					break;
				}
			}

			bytes_sent = this->client.send_frame(resend);

			if(bytes_sent == -1) {
				perror("sendto");
				continue;
			}

			cout << "resent packet " << resend.seq_num << endl;

			this->total_bytes_sent += bytes_sent;
		}

		/* timer shit */
		int current_time = this->client.get_current_time();
		for(int i = 0; i < current_window.size(); i++) {
			if(current_window[i].timer_running && current_time - current_window[i].timer_time > this->client.user.timeout_int) {
				Frame resend = current_window[i];
				resend.timer_time = current_time;

				bytes_sent = this->client.send_frame(resent);

				if(bytes_sent == -1) {
					continue;
				}

				cout << "resent packet " << resend.seq_num << endl;

				this->total_bytes_sent += bytes_sent;
			}
		}


		if(read_done) {
			bool done = true;
			for(int i = 0; i < current_window.size(); i++) {
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

	return 0;
}

int SelectiveRepeat::receive_ack(bool* nak) {
	struct pollfd fds[1];
	fds[0].fd = this->client.sockfd;
	fds[0].events = POLLIN;

	if(poll(fds, 1, 0) > 0) {
		/* soemthing to receive */
		char ack[3 + this->client.user.header_len + 1];
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
		string ack_num = string(ack).substr(3, this->client.user.header_len);
		cout << ack_or_nak << " " << ack_num << " received" << endl;

		this->packets_sent++;

		return stoi(ack_num);
	}

	return -1;
}
