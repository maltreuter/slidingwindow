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

/* string(data + seq_num) */
string Frame::to_string() {
	string sn = ::to_string(seq_num);
	int padding = 4 - sn.length();
	sn.insert(0, padding, '0');

	return data + ':' + sn;
}
