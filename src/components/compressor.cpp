#include "compressor.h"

// Based on: SimpleSource by ChunkWare Music Software
// https://github.com/music-dsp-collection/chunkware-simple-dynamics

void CompressorComponent::update(Synth const & synth) {
	auto coef_attack  = std::exp(-1000.0f / (attack  * SAMPLE_RATE));
	auto coef_release = std::exp(-1000.0f / (release * SAMPLE_RATE));

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample    = inputs[0].get_sample(i);
		auto sidechain = inputs[1].get_sample(i);

		if (inputs[1].others.size() == 0) sidechain = sample; // If there is no sidechain connected, use the input signal itself

		auto link    = std::max(std::abs(sidechain.left), std::abs(sidechain.right));
		auto link_db = util::linear_to_db(link);

		auto exceeded_db = std::max(link_db - threshold, 0.0f); // How many dB are we over the threshold?

		if (exceeded_db >= env) {
			env = exceeded_db + coef_attack * (env - exceeded_db);
		} else {
			env = exceeded_db + coef_release * (env - exceeded_db);
		}

		sample = util::db_to_linear(env * (1.0f / ratio - 1.0f) + gain) * sample;

		outputs[0].set_sample(i, sample);
	}
}

void CompressorComponent::render(Synth const & synth) {
	threshold.render(); ImGui::SameLine();
	ratio    .render(); ImGui::SameLine();
	gain     .render();

	attack .render(); ImGui::SameLine();
	release.render();
}
