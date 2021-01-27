#include "components.h"

#include "synth.h"

void DelayComponent::history_resize(int size) {
	history.resize(size);
	offset %= history.size();
}

void DelayComponent::update(Synth const & synth) {
	auto history_size = size_t(60) * SAMPLE_RATE * steps / (4 * synth.tempo);
	
	if (history.size() != history_size) {
		history_resize(history_size);
	}

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);

		sample = sample + feedback * history[offset];
		history[offset] = sample;
	
		offset = (offset + 1) % history.size();

		outputs[0].values[i] = sample;
	}
}

void DelayComponent::render(Synth const & synth) {
	steps   .render();
	feedback.render();
}
