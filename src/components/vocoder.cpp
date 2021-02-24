#include "components.h"

void VocoderComponent::calc_bands() {
	bands.resize(num_bands);
	
	static constexpr auto FREQ_START =    20.0f;
	static constexpr auto FREQ_END   = 20000.0f;

	auto scale = std::pow(FREQ_END / FREQ_START, 1.0f / float(num_bands));

	auto freq = FREQ_START;

	for (auto & band : bands) {
		band.freq = freq;
		freq *= scale;

		band.filters_mod[0].reset_state();
		band.filters_mod[1].reset_state();
		band.filters_car[0].reset_state();
		band.filters_car[1].reset_state();
	}
}

void VocoderComponent::update(Synth const & synth) {
	for (int b = 0; b < bands.size(); b++) {
		auto & band = bands[b];

		band.filters_mod[0].set(dsp::BiQuadFilterMode::BAND_PASS, band.freq, width);
		band.filters_mod[1].set(dsp::BiQuadFilterMode::BAND_PASS, band.freq, width);
		band.filters_car[0].set(dsp::BiQuadFilterMode::BAND_PASS, band.freq, width);
		band.filters_car[1].set(dsp::BiQuadFilterMode::BAND_PASS, band.freq, width);
	}

	auto decay_factor = util::log_interpolate(0.01f, 0.00001f, decay);

	auto linear_gain = util::db_to_linear(gain);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto modulator = inputs[0].get_sample(i);
		auto carrier   = inputs[1].get_sample(i);

		Sample sample = { };

		for (auto & band : bands) {
			auto mod = band.filters_mod[0].process(modulator);
			auto car = band.filters_car[0].process(carrier);

#if 0
			mod = band.filters_mod[1].process(mod);
			car = band.filters_car[1].process(car);
#endif

			auto mod_gain = std::max(std::abs(mod.left), std::abs(mod.right));

#if 1
			if (band.gain < mod_gain) {
				band.gain = mod_gain; // Gain increases instantaneously
			} else {
				band.gain = util::lerp(band.gain, mod_gain, decay_factor); // Gain decreases slowly (low passed)
			}
#else
			band.gain = util::lerp(band.gain, mod_gain, decay_factor);
#endif

			sample += band.gain * car;
		}

		outputs[0].set_sample(i, sample * linear_gain);
	}
}

void VocoderComponent::render(Synth const & synth) {
	if (num_bands.render()) {
		calc_bands();
	}

	width.render();
	decay.render();
	gain .render();
}
