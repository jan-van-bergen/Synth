#include "components.h"

#include "util.h"

void DistortionComponent::update(Synth const & synth) {
	auto threshold = 1.00001f - amount;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_sample(i);

		auto distort = [threshold](float sample) {
			return util::clamp(sample, -threshold, threshold) / threshold;
		};
		
		outputs[0].set_sample(i, Sample::apply_function(distort, sample));
	}
}

void DistortionComponent::render(Synth const & synth) {
	amount.render();
}

void DistortionComponent::serialize(json::Writer & writer) const {
	writer.write("amount", amount);
}

void DistortionComponent::deserialize(json::Object const & object) {
	amount = object.find_float("amount", amount.default_value);
}
