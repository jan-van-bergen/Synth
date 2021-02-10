#include "components.h"

#include "synth.h"

void WaveTableComponent::update(Synth const & synth) {
	if (synth.notes.size() == 0) current_sample = 0.0f;

	for (auto const & note : synth.notes) {
		auto step = util::note_freq(note.note) / util::note_freq(36);

		for (int i = 0; i < BLOCK_SIZE; i++) {	
			auto index_0 = util::round(current_sample - 0.5f);
			auto index_1 = index_0 + 1;

			if (index_1 >= samples.size()) break;

			auto weight = current_sample - std::floor(current_sample);

			outputs[0].values[i] += util::lerp(samples[index_0], samples[index_1], weight);

			current_sample += step;
		}
	}
}

void WaveTableComponent::render(Synth const & synth) {

}
