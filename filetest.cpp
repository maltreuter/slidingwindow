#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

/*

ofstream output = ofstream("./out", ofstream::binary);

char buf[packet_size];

for(int i = 0; i < n_frames - 1; i++) {
	n_bytes = recv(clientfd, buf, packet_size, 0);
	output.write(buf, packet_size);
}

output.close();

*/


int main(int argc, char *argv[]) {
	string file_path = "./test";

	vector<vector<char>> chunks;
	vector<char> buffer(128, 0);

	ifstream fin(file_path, ifstream::binary);

	int total = 0;

	while(!fin.eof()) {
		fin.read(buffer.data(), buffer.size());
		streamsize data_size = fin.gcount();

		total += data_size;

		chunks.push_back(buffer);
	}

	for(auto &chunk : chunks) {
		for(int i = 0; i < chunk.size(); i++) {
			cout << chunk[i] << endl;
		}
	}

	cout << "bytes read: " << total << endl;
}
