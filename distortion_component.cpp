#include "components.h"

#include "util.h"

void DistortionComponent::update(Synth const & synth) {
	auto threshold = 1.00001f - amount;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i) / 127.0f;

		auto distort = [threshold](float sample) {
			return util::clamp(sample, -threshold, threshold) / threshold;
		};
		
		outputs[0].values[i] = 127.0f * Sample::apply_function(distort, sample);
	}
}

void DistortionComponent::render(Synth const & synth) {
	ImGui::SliderFloat("Amount", &amount, 0.0f, 1.0f);
}
