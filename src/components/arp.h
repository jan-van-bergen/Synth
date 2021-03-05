#pragma once
#include "component.h"

struct ArpComponent : Component {
	static constexpr char const * mode_names[] = { "Up", "Down", "Up/Down", "Random" };

	enum struct Mode { UP, DOWN, UPDOWN, RANDOM } mode = Mode::UP;

	Parameter<float> steps = { this, "steps", "Stp", "Length in Steps", 1.0f, std::make_pair(0.25f, 16.0f), { 0.25f, 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 8.0f, 16.0f } };

	ArpComponent(int id) : Component(id, "Arp", { { this, "MIDI In", true } }, { { this, "MIDI Out", true } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	unsigned seed = util::seed();

	std::vector<Note> notes;

	int   current_note = 0;
	float current_time = 0.0f;

	bool going_up = true;
};
