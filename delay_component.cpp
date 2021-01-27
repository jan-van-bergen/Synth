#include "components.h"

void DelayComponent::update_history_size() {
	history.resize(115 * SAMPLE_RATE * steps / 1000);
	offset %= history.size();
}

void DelayComponent::update(Synth const & synth) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);

		sample = sample + feedback * history[offset];
		history[offset] = sample;
	
		offset = (offset + 1) % history.size();

		outputs[0].values[i] = sample;
	}
}

void DelayComponent::render(Synth const & synth) {
	if (ImGui::SliderInt("Steps", &steps, 1, 8)) {
		 update_history_size();
	}
	ImGui::SliderFloat("Feedback", &feedback, 0.0f, 1.0f);
}
