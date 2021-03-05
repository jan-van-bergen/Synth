#pragma once
#include "component.h"

struct SplitComponent : Component {
	Parameter<float> mix = { this, "mix", "A/B", "Mix A/B", 0.5f, std::make_pair(0.0f, 1.0f), { 0.0f, 0.5f, 1.0f } };

	SplitComponent(int id) : Component(id, "Split", { { this, "In" } }, { { this, "Out A" }, { this, "Out B" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
