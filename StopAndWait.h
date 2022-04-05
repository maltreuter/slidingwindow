#ifndef STOPNWAIT_H
#define STOPNWAIT_H

#include <chrono>
#include <cstring>
#include <poll.h>

#include "Client.h"

using namespace std;

class StopAndWait {
	public:
		Client client;
		int total_bytes_read;
		int total_bytes_sent;
		int packets_sent;

		StopAndWait(Client c);
		~StopAndWait();

		int send();
		int receive_ack(int send_time);
};

#endif
