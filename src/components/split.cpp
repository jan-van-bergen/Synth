#include "split.h"

void SplitComponent::update(Synth const & synth) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto const & sample = inputs[0].get_sample(i);

		outputs[0].set_sample(i, (1.0f - mix) * sample);
		outputs[1].set_sample(i,        (mix) * sample);
	}
}

void SplitComponent::render(Synth const & synth) {
	mix.render();
}
