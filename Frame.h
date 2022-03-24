#ifndef FRAME_H
#define FRAME_H

#include <iostream>
#include <string>

using namespace std;

class Frame {
	public:
		int seq_num;
		char *data;

		Frame(int seq_num, char *data);
		~Frame();
		string to_string();
};

#endif