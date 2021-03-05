#pragma once
#include "component.h"

struct VectorscopeComponent : Component {
	static constexpr auto NUM_SAMPLES = 2 * 1024;

	Sample samples[NUM_SAMPLES] = { };
	int    sample_offset = 0;

	VectorscopeComponent(int id) : Component(id, "Vectorscope", { { this, "Input" } }, { }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
