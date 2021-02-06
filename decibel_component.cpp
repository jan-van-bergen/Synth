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

void DecibelComponent::serialize(json::Writer & writer) const {
	writer.object_begin("DecibelComponent");
	writer.write("id", id);
	writer.write("pos_x", pos[0]);
	writer.write("pos_y", pos[1]);
	writer.object_end();
}

void DecibelComponent::deserialize(json::Object const & object) {

}
