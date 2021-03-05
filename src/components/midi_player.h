#pragma once
#include "component.h"

struct MIDIPlayerComponent : Component {
	static constexpr char const * DEFAULT_FILENAME = "midi/melody_2.mid";

	midi::Track midi;
	int         midi_offset;
	int         midi_length;

	std::string  filename;
	char const * filename_display;

	MIDIPlayerComponent(int id) : Component(id, "MIDI Player", { }, { { this, "MIDI Out", true } }) {
		load(DEFAULT_FILENAME);
	}

	void load(char const * file);

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
	
	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;

private:
	std::vector<Note> notes;
};
