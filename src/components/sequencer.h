#pragma once
#include "component.h"

struct SequencerComponent : Component {
	static constexpr auto TRACK_SIZE = 16;
	float pattern[TRACK_SIZE] = { };

	int current_step = 0;

	SequencerComponent(int id) : Component(id, "Sequencer", { }, { { this, "MIDI Out", true } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;
};
