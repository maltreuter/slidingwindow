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

int Client::disconnect() {
	close(this->sockfd);
	return 0;
}

// int main(int argc, char *argv[]) {
// 	if(argc != 4) {
// 		cout << "Usage: ./client <host> <port> <path_to_file>" << endl;
// 		exit(1);
// 	}
//
// 	//menu variables
// 	// string protocol;
// 	// int pkt_size;
// 	// int tmt_int;
// 	// int win_size;
// 	// int seq_range;
// 	// int errors;
// 	//
// 	// cout << "Which protocol? (BGN or SR)" << endl;
// 	// cin >> protocol;
// 	// cout << "Chosen protocol: " << protocol << endl;
// 	//
// 	// cout << "Packet size?" << endl;
// 	// cin >> pkt_size;
// 	// cout << "Chosen packet size: " << pkt_size << endl;
// 	//
// 	// cout << "Timeout interval? (ms)" << endl;
// 	// cin >> tmt_int;
// 	// cout << "Chosen timout interval: " << tmt_int << endl;
// 	//
// 	// cout << "Sliding window size?" << endl;
// 	// cin >> win_size;
// 	// cout << "Chosen sliding window size: " << win_size << endl;
// 	//
// 	// cout << "Sequence number range?" << endl;
// 	// cin >> seq_range;
// 	// cout << "Chosen sequence number range: " << seq_range << endl;;
// 	//
// 	// cout << "Situational errors?" << endl;
// 	// cout << "(0) None" << endl;
// 	// cout << "(1) Randomly generated" << endl;
// 	// cout << "(2) User specified" << endl;
// 	// cin >> errors;
// 	//
// 	// if(errors == 1) {
// 	// 	cout << "Randomly generated errors" << endl;
// 	// }
// 	// if(errors == 2) {
// 	// 	cout << "User specified errors" << endl;
// 	// }
// 	//end menu
//
//
// 	Client c = Client(argv[1], argv[2]);
// 	c.connect();
// 	c.send_file(argv[3]);
//
// 	get_md5(filesystem::path(argv[3]));
// }
