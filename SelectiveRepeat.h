#ifndef SREPEAT_H
#define SREPEAT_H

#include <chrono>
#include <cstring>
#include <poll.h>
#include <deque>

#include "Client.h"

using namespace std;

class SelectiveRepeat {
	public:
		Client client;
		int total_bytes_read;
		int total_bytes_sent;
		int packets_sent;

		SelectiveRepeat(Client c);
		~SelectiveRepeat();

		int send();
		int receive_ack(bool *nak);
};

#endif
