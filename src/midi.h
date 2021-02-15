#pragma once
#include <vector>
#include <optional>

namespace midi {
	struct Event {
		enum struct Type {
			PRESS   = 0x9,
			RELEASE = 0x8,

			CONTROL = 0xb
		} type;
						
		int time;

		union {
			struct {
				int note;
				int velocity;
			} note; // Press and Release

			struct {
				int id;
				int value;
			} control;
		};

		static Event make_press  (int time, int note, int velocity) { return { Type::PRESS,   time, note, velocity }; }
		static Event make_release(int time, int note, int velocity) { return { Type::RELEASE, time, note, velocity }; }

		static Event make_control(int time, int id, int value) { return { Type::CONTROL, time, id, value }; }
	};

	struct Track {
		int ticks = 96;
		int tempo = 130;

		std::vector<Event> events;

		static std::optional<Track> load(char const * filename);
	};

	void open();
	void close();

	std::optional<Event> get_event();
}
