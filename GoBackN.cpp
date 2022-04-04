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

	while(true) {
		/* window not full and frames to be sent */
		if(!read_done) {
			if(current_window.size() < (size_t) this->client.user.window_size) {
				f = this->client.getNextFrame(file, &read_done, next_seq_num);
				if(read_done) {
					last_frame_num = next_seq_num;
				}
				current_window.push(f);
				next_seq_num++;

				bytes_sent = this->client.send_frame(f);

				if(bytes_sent == -1) {
					continue;
				}

				cout << "sent packet " << f.seq_num << endl;

				this->total_bytes_sent += bytes_sent;
			}
		}

		int ack_num = receive_ack();

		/* if we've received an ack for the last frame: done */
		if(ack_num == last_frame_num) {
			break;
		}
		if(ack_num >= 0) {
			if(ack_num >= send_base && ack_num < next_seq_num) {
				while(send_base <= ack_num) {
					send_base++;
					current_window.pop();
				}
				timer_running = false;
			}
		} else {
			if(!timer_running) {
				timer_running = true;
				timer_time = this->client.get_current_time();
			}
		}

		/* if timer == timeout */
		if(timer_running && this->client.get_current_time() - timer_time > this->client.user.timeout_int) {
			cout << "timeout: " << this->client.get_current_time() - timer_time << endl;
			queue<Frame> tmp = current_window;
			while(!tmp.empty()) {
				Frame resend = tmp.front();
				tmp.pop();
				
				bytes_sent = this->client.send_frame(resend);

				if(bytes_sent == -1) {
					continue;
				}

				cout << "resent packet " << resend.seq_num << endl;

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

	return 0;
}

int GoBackN::receive_ack() {
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

		/* split seq_num */
		string ack_num = string(ack).substr(3, this->client.user.header_len);
		cout << "ack " << ack_num << " received" << endl;

		this->packets_sent++;

		return stoi(ack_num);
	}

	return -1;
}
