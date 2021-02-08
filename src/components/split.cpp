#include "components.h"

void SplitComponent::update(Synth const & synth) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto const & sample = inputs[0].get_value(i);

		outputs[0].values[i] = (1.0f - mix) * sample;
		outputs[1].values[i] =         mix  * sample;
	}
}

void SplitComponent::render(Synth const & synth) {
	mix.render();
}

void SplitComponent::serialize(json::Writer & writer) const {
	writer.write("mix", mix);
}

void SplitComponent::deserialize(json::Object const & object) {
	mix = object.find_float("mix", mix.default_value);
}
