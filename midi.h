#pragma once
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace midi {
	struct Event {
		bool press;

		int note;
		int velocity;

		float time;
	};

	struct Track {
		int tempo;

		std::vector<Event> events;

		static Track load(std::string const & filename);
	};

	inline std::unordered_map<int, float> controls;

	void open();
	void close();

	std::optional<Event> get_event();
}
