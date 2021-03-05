#pragma once
#include "component.h"

struct BitCrusherComponent : Component {
	Parameter<float> gain = { this, "gain", "Gain", "Gain (dB)",             0.0f, std::make_pair(0.0f, 9.0f) };
	Parameter<int>   bits = { this, "bits", "Bits", "Bits",                  32,   std::make_pair(1, 32) };
	Parameter<int>   rate = { this, "rate", "Rate", "Sample Rate Reduction",  1,   std::make_pair(1, 128) };

	BitCrusherComponent(int id) : Component(id, "Bit Crusher", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
