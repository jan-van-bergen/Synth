#include "components.h"

#include "synth.h"
#include "util.h"

static Sample generate_sine(float t, float freq, float amplitude = 1.0f) {
	return amplitude * std::sin(TWO_PI * t * freq);
}

static Sample generate_square(float t, float freq, float amplitude = 1.0f) {
	return std::fmodf(t * freq, 1.0f) < 0.5f ? amplitude : -amplitude;
}

static Sample generate_triangle(float t, float freq, float amplitude = 1.0f) {
	float x = t * freq + 0.25f;
	return amplitude * (4.0f * std::abs(x - std::floor(x + 0.5f)) - 1.0f);
}
	
static Sample generate_saw(float t, float freq, float amplitude = 1.0f) {
	return amplitude * 2.0f * (t * freq - std::floor(t * freq + 0.5f));
}

static Sample generate_noise(float amplitude = 1.0f) {
	return amplitude * ((rand() / float(RAND_MAX)) * 2.0f - 1.0f);
}

static float envelope(float t, float attack, float hold, float decay, float sustain) {
	if (t < attack) {
		return t / attack;
	}
	t -= attack;

	if (t < hold) {
		return 1.0f;
	}
	t -= hold;

	if (t < decay) {
		return util::lerp(1.0f, sustain, t / decay);
	}

	return sustain;
};

void OscilatorComponent::update(Synth const & synth) {
	for (auto const & note : synth.notes) {
		for (int i = 0; i < BLOCK_SIZE; i++) {
			auto time = synth.time + i;

			auto t = float(time - note.time) * SAMPLE_RATE_INV;

			auto frequency = util::note_freq(note.note + transpose) * std::pow(2.0f, detune / 1200.0f);
			auto amplitude = 0.2f * note.velocity * envelope(4.0f * t / 60.0f * float(synth.tempo), attack, hold, decay, sustain);

			switch (waveform_index) {
				case 0: outputs[0].values[i] += generate_sine    (t, frequency, amplitude); break;
				case 1: outputs[0].values[i] += generate_square  (t, frequency, amplitude); break;
				case 2: outputs[0].values[i] += generate_triangle(t, frequency, amplitude); break;
				case 3: outputs[0].values[i] += generate_saw     (t, frequency, amplitude); break;
				case 4: outputs[0].values[i] += generate_noise                 (amplitude); break;

				default: abort();
			}
		}
	}
}

void OscilatorComponent::render(Synth const & synth) {
	if (ImGui::BeginCombo("Waveform", waveform_names[waveform_index])) {
		for (int j = 0; j < util::array_element_count(waveform_names); j++) {
			if (ImGui::Selectable(waveform_names[j], waveform_index == j)) {
				waveform_index = j;
			}
		}

		ImGui::EndCombo();
	}

	transpose.render();
	detune   .render();

	float a = attack;
	float h = hold;
	float d = decay;
	float s = sustain;

	static constexpr auto NUM_VALUES = 100;

	float plot_values[NUM_VALUES] = { };

	for (int i = 0; i < NUM_VALUES; i++) {
		plot_values[i] = envelope(16.0f * float(i) / NUM_VALUES, attack, hold, decay, sustain);
	}

	ImGui::PlotLines("Envelope", plot_values, NUM_VALUES, 0, nullptr, 0.0f, 1.0f);
	
	attack .render();
	hold   .render();
	decay  .render();
	sustain.render();
}
