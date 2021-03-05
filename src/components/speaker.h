#pragma once
#include "component.h"

struct SpeakerComponent : Component {
	std::vector<Sample> recorded_samples;
	bool                recording = false;

	SpeakerComponent(int id) : Component(id, "Speaker", { { this, "Input" } }, { { this, "Pass" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
