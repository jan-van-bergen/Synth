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

void BitCrusherComponent::serialize(json::Writer & writer) const {
	writer.object_begin("BitCrusherComponent");
	writer.write("id", id);
	writer.write("pos_x", pos[0]);
	writer.write("pos_y", pos[1]);

	writer.write("gain", gain);
	writer.write("bits", bits);
	writer.write("rate", rate);

	writer.object_end();
}

void BitCrusherComponent::deserialize(json::Object const & object) {
	gain = object.find<json::ValueFloat const>("gain")->value;
	bits = object.find<json::ValueFloat const>("bits")->value;
	rate = object.find<json::ValueFloat const>("rate")->value;
}
