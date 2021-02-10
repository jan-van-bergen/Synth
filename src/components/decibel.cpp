#include "components.h"

void DecibelComponent::update(Synth const & synth) {
	auto max_amplitude = 0.0f;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);
		auto amplitude = std::max(std::abs(sample.left), std::abs(sample.right));

		max_amplitude = std::max(max_amplitude, amplitude);
	}

	decibels = util::linear_to_db(max_amplitude);
}

void DecibelComponent::render(Synth const & synth) {
	ImGui::Text("%f dB", decibels);
}
