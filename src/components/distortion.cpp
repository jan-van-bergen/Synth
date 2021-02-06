#include "components.h"

#include "util.h"

void DistortionComponent::update(Synth const & synth) {
	auto threshold = 1.00001f - amount;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);

		auto distort = [threshold](float sample) {
			return util::clamp(sample, -threshold, threshold) / threshold;
		};
		
		outputs[0].values[i] = Sample::apply_function(distort, sample);
	}
}

void DistortionComponent::render(Synth const & synth) {
	amount.render();
}

void DistortionComponent::serialize(json::Writer & writer) const {
	writer.object_begin("DistortionComponent");
	writer.write("id", id);
	writer.write("pos_x", pos[0]);
	writer.write("pos_y", pos[1]);

	writer.write("amount", amount);

	writer.object_end();
}

void DistortionComponent::deserialize(json::Object const & object) {
	amount = object.find<json::ValueFloat const>("amount")->value;
}
