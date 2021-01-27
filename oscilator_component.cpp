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

static float envelope(int time, float attack, float hold, float decay, float sustain) {
	auto t = float(time) * SAMPLE_RATE_INV;

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
	outputs[0].clear();

	for (auto const & [id, note] : synth.notes) {
		for (int i = 0; i < BLOCK_SIZE; i++) {
			auto time = synth.time + i;

			auto duration = time - note.time;

			auto t = float(time) * SAMPLE_RATE_INV;

			auto frequency = util::note_freq(note.note + transpose);
			auto amplitude = 20.0f * note.velocity * envelope(duration, env_attack, env_hold, env_decay, env_sustain);

			switch (waveform_index) {
				case 0: outputs[0].values[i] += generate_sine    (t, frequency, amplitude); break;
				case 1: outputs[0].values[i] += generate_square  (t, frequency, amplitude); break;
				case 2: outputs[0].values[i] += generate_triangle(t, frequency, amplitude); break;
				case 3: outputs[0].values[i] += generate_saw     (t, frequency, amplitude); break;
				case 4: outputs[0].values[i] += generate_noise(amplitude); break;

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

	ImGui::DragInt("Transpose", &transpose, 1.0f / 12.0f);

	ImGui::SliderFloat("Attack",  &env_attack,  0.0f, 4.0f);
	ImGui::SliderFloat("Hold",    &env_hold,    0.0f, 4.0f);
	ImGui::SliderFloat("Decay",   &env_decay,   0.0f, 4.0f);
	ImGui::SliderFloat("Sustain", &env_sustain, 0.0f, 1.0f);
}
