#ifndef FRAME_H
#define FRAME_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Frame {
	public:
		int seq_num;
		vector<unsigned char> data;
		int header_len;

		Frame(int seq_num, vector<unsigned char> data, int header_len);
		Frame();
		~Frame();
		string padSeqNum();
		string to_string();
};

#endif
