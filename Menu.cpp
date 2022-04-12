#include <iostream>
#include <random>
#include <filesystem>

#include "StopAndWait.h"
#include "GoBackN.h"
#include "SelectiveRepeat.h"

// #include "utils.h"

using namespace std;

vector<int> get_packets(string prompt);
vector<int> get_random_packets(int n_lose);

vector<int> get_packets(string prompt) {
	vector<int> packets = vector<int>();

	while(true) {
		cout << prompt << endl;
		string input;
		cin >> input;

		if(input == "q") {
			break;
		}
		packets.push_back(stoi(input));
	}
	cout << endl;

	return packets;
}

vector<int> get_random_packets(int n_lose, int n_packets) {
	vector<int> packets = vector<int>();

	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<> distr(0, n_packets);

	while(packets.size() < (size_t) n_lose) {
		int random = distr(gen);
		vector<int>::iterator position = find(packets.begin(), packets.end(), random);
		if(position == packets.end()) {
			packets.push_back(random);
		}
	}

	return packets;
}

int main(int argc, char *argv[]) {
	Client c = Client();

	/* Menu */
	int default_values;
	cout << "Use default values? (0 - No, 1 - Yes) ";
	cin >> default_values;

	if(default_values == 1) {
		c.user.file_path = "testrandom";
		c.user.host = "127.0.0.1";
		c.user.port = "9001";
		c.user.packet_size = 128;
		c.user.timeout_int = 5000;
		c.user.window_size = 8;
		c.user.max_seq_num = 64;
	} else {
		cout << "Enter file to send: ";
		cin >> c.user.file_path;

		cout << "Enter host: ";
		cin >> c.user.host;

		cout << "Enter port: ";
		cin >> c.user.port;

		cout << "Enter packet size: ";
		cin >> c.user.packet_size;

		cout << "Enter timeout interval (ms): ";
		cin >> c.user.timeout_int;

		cout << "Enter window size: ";
		cin >> c.user.window_size;

		cout << "Enter max sequence number: ";
		cin >> c.user.max_seq_num;
	}

	c.user.ack_len = to_string(c.user.max_seq_num).length();
	c.user.header_len += c.user.ack_len;

	int file_size = filesystem::file_size(filesystem::path(c.user.file_path));
	int n_packets = file_size / c.user.packet_size;

	cout << "Enter situational errors (0 - None, 1 - Random, 2 - User Spec): ";
	int sit_errors;
	cin >> sit_errors;

	if(sit_errors == 0) {
		c.user.errors = 0;
	} else {
		c.user.errors = 1;
	}

	if(sit_errors == 1) {
		int n_lose;
		cout << "Enter number of acks to lose (" << n_packets << " packets total) ";
		cin >> n_lose;
		c.user.lost_acks = get_random_packets(n_lose, n_packets);

		cout << "Enter number of packets to lose (" << n_packets << " packets total) ";
		cin >> n_lose;
		c.user.lost_packets = get_random_packets(n_lose, n_packets);

		cout << "Enter number of packets to corrupt (" << n_packets << " packets total) ";
		cin >> n_lose;
		c.user.corrupt_packets = get_random_packets(n_lose, n_packets);
	}

	if(sit_errors == 2) {
		string prompt = "Enter an ack to lose (nth ack) or q to stop.";
		c.user.lost_acks = get_packets(prompt);

		prompt = "Enter a packet to lose (nth packet) or q to stop.";
		c.user.lost_packets = get_packets(prompt);

		prompt = "Enter a packet (nth packet) to corrupt or q to stop.";
		c.user.corrupt_packets = get_packets(prompt);
	}

	cout << "Enter protocol (0 - Stop and Wait, 1 - Go-Back-N, 2 - Selective Repeat): ";
	cin >> c.user.protocol;
	/* end menu */

	/* reminder to check errors */
	c.connect();
	c.handshake();
	if(c.user.protocol == 0) {
		StopAndWait s = StopAndWait(c);
		s.send();
	} else if(c.user.protocol == 1) {
		GoBackN g = GoBackN(c);
		g.send();
	} else if(c.user.protocol == 2) {
		SelectiveRepeat sr = SelectiveRepeat(c);
		sr.send();
	} else {
		cout << "Not a valid protocol. Disconnecting." << endl;
	}
	c.disconnect();
}
