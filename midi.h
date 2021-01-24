#pragma once
#include <string>
#include <vector>

namespace midi {
	struct Track {
		struct Event {
			bool press;

			int note;
			int velocity;

			float time;
		};

		int tempo;

		std::vector<Event> events;

		static Track load(std::string const & filename);
	};

	void open();
	void close();
}
