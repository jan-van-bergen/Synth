#pragma once
#include "component.h"

struct OscilloscopeComponent : Component {
	float samples[BLOCK_SIZE] = { };

	OscilloscopeComponent(int id) : Component(id, "Oscilloscope", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
