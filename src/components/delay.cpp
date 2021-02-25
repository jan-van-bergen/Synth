#include "components.h"

#include "synth.h"

void DelayComponent::history_resize(int size) {
	history.resize(size);
	offset %= history.size();
}

void DelayComponent::update(Synth const & synth) {
	auto history_size = size_t(60) * SAMPLE_RATE * steps / (4 * synth.settings.tempo);
	
	if (history.size() != history_size) {
		history_resize(history_size);
	}

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_sample(i);

		sample = sample + feedback * history[offset];
		history[offset] = sample;
	
		offset = util::wrap(offset + 1, int(history.size()));

		outputs[0].set_sample(i, sample);
	}
}

void DelayComponent::render(Synth const & synth) {
	steps   .render(); ImGui::SameLine();
	feedback.render();
}
