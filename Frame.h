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
		int ack_len;
		bool timer_running;
		int timer_time;
		bool acked;
		vector<unsigned char> checksum;

		Frame(int seq_num, vector<unsigned char> data, int ack_len, vector<unsigned char> checksum);
		Frame();
		~Frame();

		string padSeqNum();
};

#endif
