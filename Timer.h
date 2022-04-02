#ifndef TIMER_H
#define TIMER_H

#include <thread>
#include <chrono>

using namespace std;

class Timer {
	bool clear = false;

public:
	template<typename Function>
	void setTimeout(Function function, int delay) {
		this->clear = false;
		thread t([=]() {
			if(this->clear) return;
			this_thread::sleep_for(chrono::milliseconds(delay));
			if(this->clear) return;
			function();
		});
		t.detach();
	}

	template<typename Function>
	void setInterval(Function function, int interval) {
		this->clear = false;
		thread t([=]() {
			while(true) {
				if(this->clear) return;
				this_thread::sleep_for(chrono::milliseconds(interval));
				if(this->clear) return;
				function();
			}
		});
		t.detach();
	}

	void stop() {
		this->clear = true;
	}
};

#endif