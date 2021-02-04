#include "components.h"

void BitCrusherComponent::update(Synth const & synth) {
	auto num_quantizations     = float(1u << (bits - 1));
	auto num_quantizations_inv = 1.0f / num_quantizations;

	for (int i = 0; i < BLOCK_SIZE; i += rate) {
		auto sample = gain * inputs[0].get_value(i); // Sample and hold

		// Quantize
		sample.left  = std::round(sample.left  * num_quantizations) / num_quantizations;
		sample.right = std::round(sample.right * num_quantizations) / num_quantizations;

		for (int j = 0; j < rate; j++) {
			auto idx = i + j;
			if (idx < BLOCK_SIZE) {
				outputs[0].values[idx] = sample;	
			}
		}
	}
}

void BitCrusherComponent::render(Synth const & synth) {
	gain.render();
	bits.render();
	rate.render();
}
