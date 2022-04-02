#include "Client.h"
#include "utils.h"

using namespace std;

int main(int argc, char *argv[]) {
	if(argc != 4) {
		cout << "Usage: ./client <host> <port> <path_to_file>" << endl;
		exit(1);
	}

	Client c = Client(string(argv[1]), string(argv[2]));
	c.connect();

	filesystem::path file_path = filesystem::path(argv[3]);

	/* menu */
	int packet_size = 128;
	int max_seq_num = 256;

	/* user input that needs to be sent to server */
	/* header struct defined in Client.h */
	header h = {
		to_string(packet_size),
		to_string(max_seq_num),
		get_md5(filesystem::path(file_path))
	};

	/* stats */
	int total_bytes_read = 0;
	int total_bytes_sent = 0;
	int packets_sent = 0;

	/* error checking fread, sendto, recvfrom */
	int bytes_read;
	int bytes_sent;
	int bytes_rcvd;

	/* send header data to server */
	bytes_sent = sendto(c.sockfd, h.packet_size.c_str(), h.packet_size.length(), 0, c.server_addr, c.server_addr_len);
	if(bytes_sent == -1) {
		perror("sendto");
		exit(1);
	}
	total_bytes_sent += bytes_sent;

	/* check if file exists */
	FILE *file = fopen(file_path.c_str(), "rb");

	vector<Frame> frames;

	bool read_done = false;

	bool resend = false;
	Frame f = Frame();
	while(!read_done) {
		if(!resend) {
			char frame_data[packet_size + 1];	/* +1 for null terminator */
			memset(frame_data, 0, packet_size + 1);

			/* read a frame from the file */
			/* fread returns packet_size on success */
			/* fread returns less than packet_size if EOF or error */
			bytes_read = fread(frame_data, 1, packet_size, file);
			if(bytes_read < packet_size && packet_size != 0) {
				if(ferror(file) != 0) {		/* error */
					perror("fread");
					exit(1);
				}
				if(feof(file) != 0) {	/* end of file = true */
					cout << "end of file caught" << endl;
					fclose(file);
					read_done = true;
				}
			}

			// cout << "bytes read: " << bytes_read << endl;
			// cout << string(frame_data) << endl;

			f = Frame(packets_sent, string(frame_data));
		}

		/* send frame 								*/
		/* 	frame: 									*/
		/*		data = packet_size bytes 			*/
		/*		seq_num = 6 bytes for "":0000\0" 	*/
		bytes_sent = sendto(c.sockfd, f.to_string().c_str(), f.to_string().length(), 0, c.server_addr, c.server_addr_len);
		if(bytes_sent == -1) {
			perror("sendto");
			continue;
		}
		total_bytes_sent += bytes_sent;

		if(resend) {
			cout << "resent packet " << f.seq_num << endl;
		} else {
			cout << "sent packet " << packets_sent << endl;
		}


		/* readfds and timeout for select() */
		fd_set select_fds;
		FD_ZERO(&select_fds);
		FD_SET(c.sockfd, &select_fds);

		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		/* select returns 0 if the timeout expires (ack not received) */
		if(select(8, &select_fds, NULL, NULL, &timeout) == 0) {
			cout << "Packet " << packets_sent << " timed out" << endl;
			resend = true;
		} else {
			/* receive ack */
			/* "ack0001\0" "ack0145\0" */
			char ack[3 + h.max_seq_num.length() + 1];
			bytes_rcvd = recvfrom(c.sockfd, ack, sizeof(ack), 0, c.server_addr, &c.server_addr_len);
			if(bytes_rcvd == -1) {
				perror("recvfrom");
				continue;
			}

			/* split seq_num */
			string ack_num = string(ack).substr(3, 4);
			cout << "ack " << ack_num << " received" << endl;

			frames.push_back(f);

			packets_sent++;
			total_bytes_read += bytes_read;
			resend = false;
		}

		/* tell the server we are done sending frames */
		/* read_done set when we got EOF from fread */
		if(read_done) {
			bytes_sent = sendto(c.sockfd, "done", 4, 0, c.server_addr, c.server_addr_len);
			if(bytes_sent == -1) {
				perror("sendto");
				exit(1);
			}
			total_bytes_sent += bytes_sent;
		}
	}

	/* before closing the connection we should receive a confirmation
	from the server that the file checksums matched */
	c.disconnect();

	/* print stats */
	cout << "\n************************************" << endl;
	cout << "Sending file: " << file_path << "\tmd5 sum: " << h.file_md5 << endl;
	cout << "packets sent: " << packets_sent << endl;
	cout << "total bytes read from file: " << total_bytes_read << endl;
	cout << "total bytes sent to server: " << total_bytes_sent << endl;

	//for(auto &frame : frames) {
		//cout << "Frame: " << frame.seq_num << endl;
		// free(frame.data);
	//}

	return 0;
}
