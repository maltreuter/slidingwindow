#ifndef GOBACKN_H
#define GOBACKN_H

#include <chrono>
#include <cstring>
#include <poll.h>
#include <queue>

#include "Client.h"

using namespace std;

class GoBackN {
	public:
		Client client;
		int total_bytes_read;
		int total_bytes_sent;
		int packets_sent;

		GoBackN(Client c);
		~GoBackN();

		int send();
		int receive_ack();
};

#endif
