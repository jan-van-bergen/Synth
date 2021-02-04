#include "components.h"

void BitCrusherComponent::update(Synth const & synth) {
	auto num_quantizations     = float(1u << (bits - 1));
	auto num_quantizations_inv = 1.0f / num_quantizations;

	Sample current_sample = { };

	for (int i = 0; i < BLOCK_SIZE; i += rate) {
		auto sample = gain * inputs[0].get_value(i);

		for (int j = 0; j < rate; j++) {
			auto idx = i + j;
			if (idx > BLOCK_SIZE) break;

			outputs[0].values[idx].left  = sample.left  - std::fmod(sample.left,  num_quantizations_inv);
			outputs[0].values[idx].right = sample.right - std::fmod(sample.right, num_quantizations_inv);
		}
	}
}

void BitCrusherComponent::render(Synth const & synth) {
	gain.render();
	bits.render();
	rate.render();
}
