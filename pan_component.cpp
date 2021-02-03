#include "components.h"

void PanComponent::update(Synth const & synth) {
	auto alpha = 0.5f + 0.5f * pan;

	// Power Pan Law: https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/
	auto left  = std::cos(0.5f * PI * alpha);
	auto right = std::sin(0.5f * PI * alpha);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);

		outputs[0].values[i].left  = left  * sample.left;
		outputs[0].values[i].right = right * sample.right;
	}
}

void PanComponent::render(Synth const & synth) {
	pan.render();
}