#include "vocoder.h"

void VocoderComponent::calc_bands() {
	bands.resize(num_bands);
	
	static constexpr auto FREQ_START =   50.0f;
	static constexpr auto FREQ_END   = 7000.0f;

	auto scale = std::pow(FREQ_END / FREQ_START, 1.0f / float(num_bands));

	auto freq = FREQ_START;

	for (auto & band : bands) {
		band.freq = freq;
		freq *= scale;

		band.filter_mod.reset_state();
		band.filter_mod.reset_state();
		band.filter_car.reset_state();
		band.filter_car.reset_state();
	}
}

void VocoderComponent::update(Synth const & synth) {
	for (int b = 0; b < bands.size(); b++) {
		auto & band = bands[b];

		band.filter_mod.set(dsp::BiQuadFilterMode::BAND_PASS, band.freq, width);
		band.filter_mod.set(dsp::BiQuadFilterMode::BAND_PASS, band.freq, width);
		band.filter_car.set(dsp::BiQuadFilterMode::BAND_PASS, band.freq, width);
		band.filter_car.set(dsp::BiQuadFilterMode::BAND_PASS, band.freq, width);
	}

	auto decay_factor = util::log_interpolate(0.01f, 0.00001f, decay);

	auto linear_gain = util::db_to_linear(gain);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto modulator = inputs[0].get_sample(i);
		auto carrier   = inputs[1].get_sample(i);

		Sample sample = { };

		for (auto & band : bands) {
			auto mod = band.filter_mod.process(modulator);
			auto car = band.filter_car.process(carrier);

			auto mod_gain = std::max(std::abs(mod.left), std::abs(mod.right));

			if (band.gain < mod_gain) {
				band.gain = mod_gain; // Gain increases instantaneously
			} else {
				band.gain = util::lerp(band.gain, mod_gain, decay_factor); // Gain decreases slowly (low passed)
			}

			sample += band.gain * car;
		}

		outputs[0].set_sample(i, sample * linear_gain);
	}
}

void VocoderComponent::render(Synth const & synth) {
	if (num_bands.render()) {
		calc_bands();
	}
	ImGui::SameLine();

	width.render();
	decay.render(); ImGui::SameLine();
	gain .render();
}

void VocoderComponent::deserialize_custom(json::Object const & object) {
	calc_bands();
}
