#pragma once
#include <string>
#include <vector>

struct MidiTrack {
	struct Event {
		bool press;

		int note;
		int velocity;

		float time;
	};

	int tempo;

	std::vector<Event> events;

	static MidiTrack load(std::string const & filename);
};
