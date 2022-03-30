#include "Frame.h"

using namespace std;

Frame::Frame(int seq_num, string data) {
	this->seq_num = seq_num;
	this->data = data;
}

Frame::Frame() {
	this->seq_num = 0;
	this->data = "";
}

Frame::~Frame() {

}

string Frame::padSeqNum() {
	string sn = ::to_string(this->seq_num);
	int padding = 4 - sn.length();
	sn.insert(0, padding, '0');
	return sn;
}

/* string(data + seq_num) */
string Frame::to_string() {
	return data + ':' + this->padSeqNum();
}
