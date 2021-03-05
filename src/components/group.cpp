#include "group.h"

void GainComponent::update(Synth const & synth) {
	auto amp = util::db_to_linear(gain);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_sample(i);
		outputs[0].set_sample(i, amp * sample);
	}
}

void GainComponent::render(Synth const & synth) {
	gain.render();
}
