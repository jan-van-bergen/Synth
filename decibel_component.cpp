#include "components.h"

void DecibelComponent::update(Synth const & synth) {
	auto max_amplitude = 0.0f;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);
		auto amplitude = std::max(std::abs(sample.left), std::abs(sample.right));

		max_amplitude = std::max(max_amplitude, amplitude);
	}

	decibels = 20.0f * std::log10(max_amplitude);
}

void DecibelComponent::render(Synth const & synth) {
	ImGui::Text("dB: %f", decibels);
}
