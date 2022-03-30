#include <iostream>
#include <stdlib.h>

#define BUF_SIZE 1024

using namespace std;

/* store md5 sum in a temp file and read into a string */
/* will need to change to md5sum on poseidon */
string get_md5(filesystem::path file_path) {
	char buffer[BUF_SIZE];
	memset(buffer, 0, BUF_SIZE);

	string command = "md5 -q " + file_path.filename().string() + " > md5.txt";
	system(command.c_str());

	FILE *file = fopen("md5.txt", "rb");
	int bytes_read = fread(buffer, 1, BUF_SIZE - 1, file);
	if(bytes_read < BUF_SIZE - 1 && BUF_SIZE != 0) {
		if(ferror(file) != 0) {		/* error */
			perror("fread");
			exit(1);
		}
		if(feof(file) != 0) {	/* end of file = true */
			fclose(file);
		}
	}

	system("rm md5.txt");

	return string(buffer);
}