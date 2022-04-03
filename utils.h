#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <chrono>
#include <stdlib.h>
#include <filesystem>

#define BUF_SIZE 1024

using namespace std;

typedef struct header {
	string packet_size;
	string header_len;
	string file_md5;
} header;

int get_current_time() {
	return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
}

/* store md5 sum in a temp file and read into a string */
/* will need to change to md5sum on poseidon */
string get_md5(filesystem::path file_path) {
	char buffer[BUF_SIZE];
	memset(buffer, 0, BUF_SIZE);
	string md5;

	string tmp = file_path.filename().string() + "md5.txt";

	string command = "md5sum " + file_path.filename().string() + " | cut -d ' ' -f 1 > " + tmp;
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

#endif
