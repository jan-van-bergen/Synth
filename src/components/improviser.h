#pragma once
#include "component.h"

struct ImproviserComponent : Component {
	Parameter<int> tonality = { this, "tonality", "Ton", "Tonality", 0, std::make_pair(0, 11) };

	static constexpr char const * mode_names[] = { "Major", "Minor" };
	enum struct Mode { MAJOR, MINOR } mode = Mode::MAJOR;

	Parameter<int> num_notes = { this, "num_notes", "Num", "Number of notes in chord", 3, std::make_pair(1, 5) };

	ImproviserComponent(int id) : Component(id, "Improviser", { }, { { this, "MIDI Out", true } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
	
	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;

private:
	unsigned seed = util::seed();

	std::vector<int> chord;
	int current_chord = 0;
	int current_time  = std::numeric_limits<int>::max();
};
