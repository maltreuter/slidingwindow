#include "utils.h"

using namespace std;

int get_current_time() {
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

/* store md5 sum in a temp file and read into a string */
string get_md5(string file_path) {
	char buffer[BUF_SIZE];
	memset(buffer, 0, BUF_SIZE);
	string md5;

	string tmp = file_path + "md5.txt";

	string command = "md5sum " + file_path + " | cut -d ' ' -f 1 > " + tmp;
	system(command.c_str());

	FILE *file = fopen(tmp.c_str(), "rb");
	int bytes_read = fread(buffer, 1, BUF_SIZE - 1, file);
	if(bytes_read) {
		if(ferror(file) != 0) {		/* error */
			perror("fread");
			exit(1);
		}
		if(feof(file) != 0) {	/* end of file = true */
			fclose(file);
		}
	}

	command = "rm " + tmp;
	system(command.c_str());

	md5 = string(buffer);

	/* strip new line */
	size_t pos = md5.find("\n");
	if(pos != string::npos) {
		md5.erase(pos, 1);
	}

	return md5;
}

string uchar_to_binary(unsigned char c) {
	char binary[9];
	char *bp = binary;

	for(int i = 7; i >= 0; i--) {
		bp += sprintf(bp, "%d", (c >> i) & 1);
	}

	return string(binary);
}

string vector_to_string(vector<int> packets) {
	string result;
	for(size_t i = 0; i < packets.size(); i++) {
		result += to_string(packets[i]) + " ";
	}

	return result;
}

vector<int> string_to_vector(string packets) {
	vector<int> result = vector<int>();
	string temp;
	stringstream ss(packets);

	while(getline(ss, temp, ' ')) {
		result.push_back(stoi(temp));
	}

	return result;
}
