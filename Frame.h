#ifndef FRAME_H
#define FRAME_H

#include <iostream>
#include <string>

using namespace std;

class Frame {
	public:
		int seq_num;
		string data;

		Frame(int seq_num, string data);
		Frame();
		~Frame();
		string to_string();
};

#endif
