#pragma once
#include <string>
#include <vector>
#include <span>

#include <ImGui/imgui.h>

#include "sample.h"
#include "note_event.h"

struct Component;

struct Connector {
	static constexpr auto RENDER_SIZE = 16.0f;

	bool const is_input;
	bool const is_midi;

	Component * component;

	std::string name;

	ImVec2 pos;

	Connector(bool is_input, bool is_midi, Component * component, std::string const & name) : is_input(is_input), is_midi(is_midi), component(component), name(name) { }
};

struct ConnectorIn : Connector {
	std::vector<std::pair<struct ConnectorOut *, float>> others; 
	
	ConnectorIn(Component * component, std::string const & name, bool is_midi = false) : Connector(true, is_midi, component, name) { }

	Sample get_sample(int i) const;

	std::vector<NoteEvent> get_events() const;
};

struct ConnectorOut : Connector {
	std::vector<ConnectorIn *> others;
	
	unsigned char data[BLOCK_SIZE * sizeof(Sample)];

	ConnectorOut(Component * component, std::string const & name, bool is_midi = false) : Connector(false, is_midi, component, name) {
		clear();
	}
	
	void clear() {
		memset(data, 0, sizeof(data));

		num_midi_events = 0;
	}

	Sample       & get_sample(int i)       { assert(!is_midi); return reinterpret_cast<Sample       *>(data)[i]; }
	Sample const & get_sample(int i) const { assert(!is_midi); return reinterpret_cast<Sample const *>(data)[i]; }

	void set_sample(int i, Sample sample) { assert(!is_midi); reinterpret_cast<Sample *>(data)[i] = sample; }

	void add_event(NoteEvent note_event);
	
	std::span<NoteEvent const> get_events() const;

private:
	int num_midi_events;
};
