#include "Frame.h"

using namespace std;

Frame::Frame(int seq_num, vector<unsigned char> data, int header_len) {
	this->seq_num = seq_num;
	this->data = data;
	this->header_len = header_len;
	this->timer_running = false;
	this->timer_time = 0;
	this->acked = false;
}

Frame::Frame() {
	this->seq_num = 0;
	this->data = vector<unsigned char>();
	this->header_len = 8;
	this->timer_running = false;
	this->timer_time = 0;
	this->acked = false;
}

Frame::~Frame() {

}

string Frame::padSeqNum() {
	string sn = ::to_string(this->seq_num);
	int padding = this->header_len - sn.length();
	sn.insert(0, padding, '0');
	return sn;
}

/* string(data + seq_num) */
string Frame::to_string() {
	// return this->padSeqNum() + data;
	return "borked";
}
