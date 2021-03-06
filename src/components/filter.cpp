#include "filter.h"

#include <ImGui/font_audio.h>

#include "util.h"

void FilterComponent::update(Synth const & synth) {
	dsp::VAFilterMode mode;
	switch (filter_type) {
		case 0: mode = dsp::VAFilterMode::LOW_PASS;  break;
		case 1: mode = dsp::VAFilterMode::HIGH_PASS; break;
		case 2: mode = dsp::VAFilterMode::BAND_PASS; break;
		case 3: mode = dsp::VAFilterMode::OFF;       break;

		default: abort();
	}

	filter.set(mode, cutoff, resonance);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_sample(i);
		sample = filter.process(sample);
		outputs[0].set_sample(i, sample);
	}
}

void FilterComponent::render(Synth const & synth) {
	auto fmt_filter = [](int value, char * fmt, int len) {
		switch (value) {
			case 0: strcpy_s(fmt, len, ICON_FAD_FILTER_LOWPASS);  break;
			case 1: strcpy_s(fmt, len, ICON_FAD_FILTER_HIGHPASS); break;
			case 2: strcpy_s(fmt, len, ICON_FAD_FILTER_BANDPASS); break;
			case 3: strcpy_s(fmt, len, ICON_FAD_FILTER_BYPASS);   break;

			default: abort();
		}
	};

	filter_type.render(fmt_filter); ImGui::SameLine();
	cutoff     .render();           ImGui::SameLine();
	resonance  .render();
}
