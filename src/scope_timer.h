#pragma once
#include <chrono>

// Timer that records the time between its construction and destruction
struct ScopeTimer {
private:
	const char * name;
	std::chrono::high_resolution_clock::time_point start_time;

public:
	inline ScopeTimer(const char * name) : name(name) {
		start_time = std::chrono::high_resolution_clock::now();
	}

	inline ~ScopeTimer() {
		auto stop_time = std::chrono::high_resolution_clock::now();
		auto duration  = std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time).count();

		if (duration >= 60000000) {
			printf("%s took: %llu s (%llu min)\n", name, duration / 1000000, duration / 60000000);
		} else if (duration >= 1000000) {
			printf("%s took: %llu us (%llu s)\n", name, duration, duration / 1000000);
		} else if (duration >= 1000) {
			printf("%s took: %llu us (%llu ms)\n", name, duration, duration / 1000);
		} else {
			printf("%s took: %llu us\n", name, duration);
		}
	}
};
