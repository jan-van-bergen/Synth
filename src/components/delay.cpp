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
	
		offset = (offset + 1) % history.size();

		outputs[0].set_sample(i, sample);
	}
}

void DelayComponent::render(Synth const & synth) {
	steps   .render();
	feedback.render();
}

void DelayComponent::serialize(json::Writer & writer) const {
	writer.write("steps",    steps);
	writer.write("feedback", feedback);
}

void DelayComponent::deserialize(json::Object const & object) {
	steps    = object.find_int  ("steps",    steps   .default_value);
	feedback = object.find_float("feedback", feedback.default_value);
						
	memset(history.data(), 0, history.size() * sizeof(Sample)); // Clear history
}
