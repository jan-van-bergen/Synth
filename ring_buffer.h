#pragma once
#include <atomic>

template<typename T, int N>
struct RingBuffer {
private:
	T buffers[N];

	std::atomic<int> read  = 0;
	std::atomic<int> write = 0;

	int curr_read  = 0; // TODO: FALSE SHARING
	int curr_write = 0;

public:
	bool can_read() const {
		return read.load() != write.load();
	}

	T const & get_read() { 
		curr_read = read.load();
		while (curr_read == write.load()) { }

		return buffers[curr_read];
	}
	
	void advance_read() {
		read.store((curr_read + 1) % N);
	}

	T & get_write() { 
		curr_write = write.load();

		return buffers[curr_write];
	}

	void advance_write() {
		auto next_write = (curr_write + 1) % N;
		while (next_write == read.load()) { }

		write.store(next_write);
	}
};
