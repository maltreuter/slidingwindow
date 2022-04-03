#include <iostream>

#include "StopAndWait.h"

using namespace std;

int main(int argc, char *argv[]) {
	Client c = Client();

	/* Menu */
	cout << "Enter file to send: ";
	cin >> c.user.file_path;

	cout << "Enter host: ";
	cin >> c.user.host;

	cout << "Enter port: ";
	cin >> c.user.port;

	cout << "Enter packet size: ";
	cin >> c.user.packet_size;

	cout << "Enter timeout interval (ms): ";
	cin >> c.user.timeout_int;

	cout << "Enter window size: ";
	cin >> c.user.window_size;

	cout << "Enter situational errors (0 - None, 1 - Random, 2 - User Spec): ";
	cin >> c.user.errors;

	cout << "Enter protocol (0 - Stop and wait, 1 - Go-Back-N, 2 - Selective Repeat): ";
	cin >> c.user.protocol;
	/* end menu */

	/* reminder to check errors */
	c.connect();
	c.handshake();
	StopAndWait s = StopAndWait(c);
	s.send();
	c.disconnect();
}
