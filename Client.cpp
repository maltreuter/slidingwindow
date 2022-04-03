#include "Client.h"

using namespace std;

Client::Client() {
	this->user = {
		"", /* file path */
		"", /* host */
		"", /* port */
		-1, /* packet_size */
		-1, /* timeout interval */
		-1, /* window size */
		-1, /* situational errors */
		-1, /* protocol */
		8
	};

	this->sockfd = -1;
}

Client::~Client() {

}

int Client::connect() {
	struct addrinfo hints, *address_info, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	getaddrinfo(this->user.host.c_str(), this->user.port.c_str(), &hints, &address_info);

	for(p = address_info; p != NULL; p = p->ai_next) {
		this->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(this->sockfd == -1) {
			perror("socket");
			continue;
		}

		cout << "Connected" << endl;
		this->server_addr = p->ai_addr;
		this->server_addr_len = p->ai_addrlen;
		return 0;
	}
	return -1;
}

int Client::disconnect() {
	close(this->sockfd);
	return 0;
}

int Client::handshake() {
	/* send packet size to server */
	// cout << this->user.packet_size << endl;
	// cout << this->user.header_len << endl;

	int bytes_sent = sendto(this->sockfd,
		to_string(this->user.packet_size).c_str(),
		to_string(this->user.packet_size).length(),
		0,
		this->server_addr,
		this->server_addr_len
	);
	if(bytes_sent == -1) {
		perror("sendto");
		return -1;
	}

	/* send header len to server */
	bytes_sent = sendto(this->sockfd,
		to_string(this->user.header_len).c_str(),
		to_string(this->user.header_len).length(),
		0,
		this->server_addr,
		this->server_addr_len
	);
	if(bytes_sent == -1) {
		perror("sendto");
		return -1;
	}
	return 0;
}

int Client::get_current_time() {
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

// int Client::runProtocol() {
// 	StopAndWait s = StopAndWait(this);
// 	s.send();
// 	return 0;
// }

// int main(int argc, char *argv[]) {
// 	if(argc != 4) {
// 		cout << "Usage: ./client <host> <port> <path_to_file>" << endl;
// 		exit(1);
// 	}
//
// 	//menu variables

//
//
// 	Client c = Client(argv[1], argv[2]);
// 	c.connect();
// 	c.send_file(argv[3]);
//
// 	get_md5(filesystem::path(argv[3]));
// }
