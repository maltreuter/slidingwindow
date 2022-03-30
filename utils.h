#include <iostream>
#include <stdlib.h>

#define BUF_SIZE 1024

using namespace std;

/* store md5 sum in a temp file and read into a string */
/* will need to change to md5sum on poseidon */
string get_md5(filesystem::path file_path) {
	char buffer[BUF_SIZE];
	memset(buffer, 0, BUF_SIZE);
	string md5;

	string tmp = file_path.filename().string() + "md5.txt";

	string command = "md5 -q " + file_path.filename().string() + " > " + tmp;
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