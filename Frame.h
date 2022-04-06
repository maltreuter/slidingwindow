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
		bool timer_running;
		int timer_time;
		bool acked;
		string checksum;

		Frame(int seq_num, vector<unsigned char> data, int header_len);
		Frame();
		~Frame();

		string padSeqNum();
};

#endif
