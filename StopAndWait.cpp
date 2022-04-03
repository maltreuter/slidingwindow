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
	Frame f = Frame();
	while(!read_done) {
		if(!resend) {
			f = getNextFrame(file, &read_done);
		}

		/* package frame for delivery */
		int buffer_size = this->client.user.header_len + this->client.user.packet_size + 1;
		unsigned char curr_frame[buffer_size];
		memcpy(curr_frame, f.padSeqNum().c_str(), this->client.user.header_len + 1);
		memcpy(curr_frame + this->client.user.header_len + 1, f.data.data(), this->client.user.packet_size);

		int send_time = this->client.get_current_time();

		/* send current frame */
		bytes_sent = sendto(this->client.sockfd,
			curr_frame,
			this->client.user.header_len + 1 + f.data.size(),
			0,
			this->client.server_addr,
			this->client.server_addr_len
		);
		if(bytes_sent == -1) {
			perror("sendto");
			resend = true;
			continue;
		}
		this->total_bytes_sent += bytes_sent;

		if(resend) {
			cout << "resent packet " << f.seq_num << endl;
		} else {
			cout << "sent packet " << f.seq_num << endl;
		}

		resend = receive_ack(send_time);
		if(!resend) {
			frames.push_back(f);
			this->total_bytes_read += f.data.size();
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

Frame StopAndWait::getNextFrame(FILE* file, bool* read_done) {
	unsigned char frame_data[this->client.user.packet_size];
	memset(frame_data, 0, this->client.user.packet_size);

	/* read a frame from the file */
	/* fread returns packet_size on success */
	/* fread returns less than packet_size if EOF or error */
	int bytes_read = fread(frame_data, 1, this->client.user.packet_size, file);
	if(bytes_read < this->client.user.packet_size && this->client.user.packet_size != 0) {
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
	return Frame(this->packets_sent, data, this->client.user.header_len);
}

bool StopAndWait::receive_ack(int send_time) {
	struct pollfd fds[1];
	fds[0].fd = this->client.sockfd;
	fds[0].events = POLLIN;

	/* Receive Ack */
	while(true) {
		int now = this->client.get_current_time();
		if(now - send_time > this->client.user.timeout_int) {
			cout << "Timeout: " << now - send_time << endl;
			cout << "Packet " << this->packets_sent << " timed out" << endl;
			return true;
		} else if(poll(fds, 1, 0) > 0) {
			/* receive ack */
			/* "ack0001\0" "ack9945\0" */
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
				continue;
			}

			/* split seq_num */
			string ack_num = string(ack).substr(3, this->client.user.header_len);
			cout << "ack " << ack_num << " received" << endl;

			this->packets_sent++;

			return false;
		}
	}
}

int StopAndWait::receive() {
	return 0;
}

// int main(int argc, char *argv[]) {
// 	if(argc != 4) {
// 		cout << "Usage: ./client <host> <port> <path_to_file>" << endl;
// 		exit(1);
// 	}
//
// 	int runtime = 0;
// 	Timer t = Timer();
// 	t.setInterval([&]() {
// 		cout << "Runtime++" << endl;
// 		runtime++;
// 	}, 1000);
//
// 	Client c = Client(string(argv[1]), string(argv[2]));
// 	c.connect();
//
// 	filesystem::path file_path = filesystem::path(argv[3]);
//
// 	/* menu */
// 	int packet_size = 128;
// 	int header_len = 8;
//
// 	/* user input that needs to be sent to server */
// 	/* header struct defined in utils.h */
// 	header h = {
// 		to_string(packet_size),
// 		to_string(header_len),
// 		get_md5(filesystem::path(file_path))
// 	};
//
//
// }
