#include "Frame.h"

using namespace std;

Frame::Frame(int seq_num, vector<unsigned char> data, int ack_len, vector<unsigned char> checksum) {
	this->seq_num = seq_num;
	this->data = data;
	this->ack_len = ack_len;
	this->timer_running = false;
	this->timer_time = 0;
	this->acked = false;
	this->checksum = checksum;
	this->packet_num = -1;
}

Frame::Frame() {
	this->seq_num = 0;
	this->data = vector<unsigned char>();
	this->ack_len = 0;
	this->timer_running = false;
	this->timer_time = 0;
	this->acked = false;
	this->checksum = vector<unsigned char>();
	this->packet_num = -1;
}

Frame::~Frame() {

}

string Frame::padSeqNum() {
	string sn = ::to_string(this->seq_num);
	int padding = this->ack_len - sn.length();
	sn.insert(0, padding, '0');
	return sn;
}
