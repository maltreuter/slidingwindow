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
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(this->host.c_str(), this->port.c_str(), &hints, &address_info);

	for(p = address_info; p != NULL; p = p->ai_next) {
		this->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(this->sockfd == -1) {
			perror("socket");
			continue;
		}
		if(::connect(this->sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("connect");
			continue;
		}

		cout << "Connected" << endl;
		break;

	}
}

int Client::send_file(string file_path) {
	int packet_size = 128;
	int n_frames;
	filesystem::path p = file_path;
	int file_size = filesystem::file_size(p);

	vector<char> buffer(packet_size, 0);

	ifstream fin(file_path, ifstream::binary);

	n_frames = file_size / packet_size;
	if(file_size % packet_size != 0) {
		n_frames++;
	}

	int total = 0;

	send(this->sockfd, &n_frames, sizeof(n_frames), 0);
	send(this->sockfd, &packet_size, sizeof(packet_size), 0);

	while(!fin.eof()) {
		fin.read(buffer.data(), buffer.size());
		streamsize data_size = fin.gcount();

		total += data_size;
		send(this->sockfd, buffer.data(), buffer.size(), 0);
	}

	close(this->sockfd);

	cout << "bytes read: " << total << endl;
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
