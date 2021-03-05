#pragma once
#include "component.h"

struct SpectrumComponent : Component {
	static constexpr auto N = 4 * 1024;

	float magnitudes[N / 2] = { };

	SpectrumComponent(int id) : Component(id, "Spectrum", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
