#pragma once
#include "component.h"

struct GainComponent : Component {
	Parameter<float> gain = { this, "gain", "Gain", "Gain (dB)", 0.f, std::make_pair(-30.0f, 30.0f), { 0.0f } };

	GainComponent(int id) : Component(id, "Gain", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
