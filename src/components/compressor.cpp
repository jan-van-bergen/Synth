#include "components.h"

// Based on: SimpleSource by ChunkWare Music Software
// https://github.com/music-dsp-collection/chunkware-simple-dynamics

void CompressorComponent::update(Synth const & synth) {
	auto coef_attack  = std::exp(-1000.0f / (attack  * SAMPLE_RATE));
	auto coef_release = std::exp(-1000.0f / (release * SAMPLE_RATE));

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);

		auto link    = std::max(std::abs(sample.left), std::abs(sample.right));
		auto link_db = util::linear_to_db(link);

		auto exceeded_db = std::max(link_db - threshold, 0.0f); // How many dB are we over the threshold?

		if (exceeded_db >= env) {
			env = exceeded_db + coef_attack * (env - exceeded_db);
		} else {
			env = exceeded_db + coef_release * (env - exceeded_db);
		}
		
		outputs[0].values[i] = util::db_to_linear(env * (1.0f / ratio - 1.0f) + gain) * sample;
	}
}

void CompressorComponent::render(Synth const & synth) {
	threshold.render();
	ratio    .render();
	gain     .render();

	attack .render();
	release.render();
}

void CompressorComponent::serialize(json::Writer & writer) const {
	writer.write("threshold", threshold);
	writer.write("ratio",     ratio);
	writer.write("gain",      gain);

	writer.write("attack",  attack);
	writer.write("release", release);
}

void CompressorComponent::deserialize(json::Object const & object) {
	threshold = object.find<json::ValueFloat const>("threshold")->value;
	ratio     = object.find<json::ValueFloat const>("ratio")    ->value;
	gain      = object.find<json::ValueFloat const>("gain")     ->value;

	attack  = object.find<json::ValueFloat const>("attack") ->value;
	release = object.find<json::ValueFloat const>("release")->value;
}
