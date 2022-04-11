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

/* package bits into uchars for sending in header */
void uint16bits_to_uchars(uint16_t input, unsigned char *output) {
	output[0] = (input >> 8) & 0x00FF;
	output[1] = input & 0x00FF;
}

/* unpack the uchars that were sent back into a uint16_t */
/* need to do this for final addition of the 2 checksums */
uint16_t uchars_to_uint16(unsigned char *input) {
	uint16_t word = ((input[0] << 8) & 0xFF00) + (input[1] & 0xFF);
	return word;
}

/* client will create a checksum and store ~checksum as 2 unsigned characters */
/* these 2 unsigned characters will be put into the header of a frame and sent */
/* server will create its own checksum with the data it received */
/* and verify that client_cs + server_cs == 1111111111111111 */
uint16_t create_checksum(unsigned char *data, int size) {
	int padding = 0;
	if(size % 2 != 0) {
		padding = 1;
	}
	unsigned char buff[size + padding];
	memcpy(buff, data, size);
	if(padding) {
		buff[size] = '0';
	}

	uint32_t sum = 0;
	uint16_t word;

	for(int i = 0; i < sizeof(buff); i=i+2) {
		word = ((buff[i] << 8) & 0xFF00) + (buff[i + 1] & 0xFF);
		sum = sum + (uint32_t) word;
	}

	while(sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	// sum = ~sum;

	return (uint16_t) sum;
}