#include "Client.h"

using namespace std;

Client::Client(string host, string port) {
	this->host = host;
	this->port = port;
	this->sockfd = -1;
}

Client::~Client() {

}

void Client::connect() {
	struct addrinfo hints, *address_info, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfo(this->host.c_str(), this->port.c_str(), &hints, &address_info);

	for(p = address_info; p != NULL; p = p->ai_next) {
		this->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(this->sockfd == -1) {
			perror("socket");
			continue;
		}

		cout << "Connected" << endl;
		this->server_addr = p->ai_addr;
		this->server_addr_len = p->ai_addrlen;
		break;

	}
}

int Client::send_file(string file_path) {
	/* user input */
	int packet_size = 128;

	int total = 0;
	int packets_sent = 0;
	int bytes_read;
	int bytes_sent;
	int bytes_rcvd;

	bytes_sent = sendto(this->sockfd, to_string(packet_size).c_str(), to_string(packet_size).length(), 0, this->server_addr, this->server_addr_len);
	if(bytes_sent == -1) {
		perror("sendto");
		exit(1);
	}

	/* check if file exists */
	FILE *file = fopen(file_path.c_str(), "rb");

	vector<Frame> frames;

	bool read_done = false;

	while(!read_done) {
		char *frame_data = (char*)malloc(packet_size);
		memset(frame_data, 0, packet_size);

		/* read a frame from the file */
		bytes_read = fread(frame_data, 1, packet_size, file);
		// cout << "bytes read: " << bytes_read << endl;

		Frame f = Frame(packets_sent, frame_data);
		string current_frame = f.to_string();

		bytes_sent = sendto(this->sockfd, current_frame.c_str(), packet_size + 5, 0, this->server_addr, this->server_addr_len);
		if(bytes_sent == -1) {
			perror("sendto");
			continue;
		}

		// cout << "bytes sent: " << bytes_sent << endl;
		cout << "sent packet " << packets_sent << endl;
		// cout << current_frame << endl;

		/* receive ack  - check for errors*/
		char ack[8];
		bytes_rcvd = recvfrom(this->sockfd, ack, 8, 0, this->server_addr, &this->server_addr_len);
		cout << "ack " << ack << " received" << endl;

		frames.push_back(f);

		packets_sent++;
		total += bytes_read;

		// check errors in sendto
		if(bytes_read < packet_size) {
			bytes_sent = sendto(this->sockfd, "done", 4, 0, this->server_addr, this->server_addr_len);
			read_done = true;
		}
	}

	fclose(file);
	close(this->sockfd);

	cout << "packets sent: " << packets_sent << endl;
	cout << "bytes read: " << total << endl;

	for(auto &frame : frames) {
		//cout << "Frame: " << frame.seq_num << endl;
		free(frame.data);
	}

	return 0;
}

int Client::receive_file() {
	cout << "Receive file" << endl;
	return 0;
}

int Client::send_cin() {
	char buf[1024];
	int nbytes;

	bool exit = 0;

	while(!exit) {
		for(string line; getline(cin, line);) {
			if(strcmp(line.c_str(), "exit") == 0) {
				exit = 1;
			}
			send(this->sockfd, line.c_str(), line.length(), 0);
			if(exit) {
				break;
			}
		}
	}

	nbytes = recv(this->sockfd, buf, 1023, 0);
	buf[nbytes] = '\0';

	cout << "Received: '" << buf << "'" << endl;

	return close(this->sockfd);
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		cout << "Usage: ./client <host> <port>" << endl;
		exit(1);
	}

	//menu variables
	// string protocol;
	// int pkt_size;
	// int tmt_int;
	// int win_size;
	// int seq_range;
	// int errors;
	//
	// cout << "Which protocol? (BGN or SR)" << endl;
	// cin >> protocol;
	// cout << "Chosen protocol: " << protocol << endl;
	//
	// cout << "Packet size?" << endl;
	// cin >> pkt_size;
	// cout << "Chosen packet size: " << pkt_size << endl;
	//
	// cout << "Timeout interval? (ms)" << endl;
	// cin >> tmt_int;
	// cout << "Chosen timout interval: " << tmt_int << endl;
	//
	// cout << "Sliding window size?" << endl;
	// cin >> win_size;
	// cout << "Chosen sliding window size: " << win_size << endl;
	//
	// cout << "Sequence number range?" << endl;
	// cin >> seq_range;
	// cout << "Chosen sequence number range: " << seq_range << endl;;
	//
	// cout << "Situational errors?" << endl;
	// cout << "(0) None" << endl;
	// cout << "(1) Randomly generated" << endl;
	// cout << "(2) User specified" << endl;
	// cin >> errors;
	//
	// if(errors == 1) {
	// 	cout << "Randomly generated errors" << endl;
	// }
	// if(errors == 2) {
	// 	cout << "User specified errors" << endl;
	// }
	//end menu


	Client c = Client(argv[1], argv[2]);
	c.connect();
	c.send_file("./test");
}
