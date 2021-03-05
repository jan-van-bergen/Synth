#pragma once
#include "component.h"

struct DistortionComponent : Component {
	Parameter<float> amount = { this, "amount", "Amt", "Amount", 0.5f, std::make_pair(0.0f, 1.0f) };

	DistortionComponent(int id) : Component(id, "Distortion", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
