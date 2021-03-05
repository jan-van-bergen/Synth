#pragma once
#include "component.h"

struct PanComponent : Component {
	Parameter<float> pan = { this, "pan", "Pan", "Pan", 0.0f, std::make_pair(-1.0f, 1.0f), { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f } };

	PanComponent(int id) : Component(id, "Pan", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
