#pragma once
#include "component.h"

struct DelayComponent : Component {
	Parameter<int>   steps    = { this, "steps",    "Del", "Delay in Steps",  3,    std::make_pair(1, 8) };
	Parameter<float> feedback = { this, "feedback", "FB",  "Feedback Amount", 0.7f, std::make_pair(0.0f, 1.0f) };

	std::vector<Sample> history;
	int offset = 0;

	DelayComponent(int id) : Component(id, "Delay", { { this, "In" } }, { { this, "Out" } }) { }

	void history_resize(int size);
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
