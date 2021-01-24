#pragma once
#include <string>
#include <vector>
#include <optional>

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

	void open();
	void close();

	std::optional<Event> get_event();
}
