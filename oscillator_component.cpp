#include "components.h"

#include "synth.h"
#include "util.h"

static Sample generate_sine(float phase) {
	return std::sin(TWO_PI * phase);
}

static Sample generate_square(float phase) {
	return std::fmodf(phase, 1.0f) < 0.5f ? 1.0f : -1.0f;
}

static Sample generate_triangle(float phase) {
	auto x = phase + 0.25f;
	return 4.0f * std::abs(x - std::floor(x + 0.5f)) - 1.0f;
}
	
static Sample generate_saw(float phase) {
	return 2.0f * (phase - std::floor(phase + 0.5f));
}

static Sample generate_noise() {
	return (rand() / float(RAND_MAX)) * 2.0f - 1.0f;
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

void OscillatorComponent::update(Synth const & synth) {
	auto steps_per_second = 4.0f / 60.0f * float(synth.tempo);

	// Check for pressed notes
	for (auto const & note : synth.notes) {
		auto voice = std::find_if(voices.begin(), voices.end(), [note = note.note](auto voice) { return voice.note == note && std::isinf(voice.release_time); });
		if (voice == voices.end()) {
			voices.emplace_back(note.note, note.velocity);
		}
	}

	// Check for released notes
	for (auto & voice : voices) {
		auto note = std::find_if(synth.notes.begin(), synth.notes.end(), [voice_note = voice.note](auto note) { return note.note == voice_note; });
		if (note == synth.notes.end() && std::isinf(voice.release_time)) {
			voice.release_time = voice.sample * SAMPLE_RATE_INV * steps_per_second;
		}
	}

	struct PortamentoState {
		int   index;
		float frequency;
	};

	PortamentoState portamento_max = { -1, portamento_frequency };

	for (int v = 0; v < voices.size(); v++) {
		auto & voice = voices[v];

		auto released = voice.release_time < INFINITY;

		auto note_frequency = util::note_freq(voice.note + transpose) * std::pow(2.0f, detune / 1200.0f);

		PortamentoState portamento_voice = { 0, portamento_frequency };
		
		for (int i = 0; i < BLOCK_SIZE; i++) {
			auto t     = voice.sample * SAMPLE_RATE_INV;
			auto steps = t * steps_per_second;

			// Apply envelope
			auto amplitude = voice.velocity * envelope(steps, attack, hold, decay, sustain);

			// Fade after release
			if (released) {
				auto steps_since_release = steps - voice.release_time;

				if (steps_since_release < release) {
					amplitude = util::lerp(amplitude, 0.0f, steps_since_release / release);
				} else {
					// Fade has run out, remove Voice
					voices.erase(voices.begin() + v);
					v--;

					break;
				}
			}

			// Generate selected waveform
			switch (waveform_index) {
				case 0: outputs[0].values[i] += amplitude * generate_sine    (voice.phase); break;
				case 1: outputs[0].values[i] += amplitude * generate_square  (voice.phase); break;
				case 2: outputs[0].values[i] += amplitude * generate_triangle(voice.phase); break;
				case 3: outputs[0].values[i] += amplitude * generate_saw     (voice.phase); break;
				case 4: outputs[0].values[i] += amplitude * generate_noise(); break;

				default: abort();
			}
			
			float frequency;

			// Apply portamento
			if (steps < portamento) {
				frequency = util::lerp(portamento_frequency, note_frequency, steps / portamento);
			} else {
				frequency = note_frequency;
			}

			// Advance phase of the wave
			auto phase_delta = frequency * SAMPLE_RATE_INV;
			voice.phase = std::fmod(voice.phase + phase_delta, 1.0f);

			voice.sample += 1.0f;

			if (!released) {
				portamento_voice.index = i;
				portamento_voice.frequency = frequency;
			}
		}

		if (portamento_voice.index > portamento_max.index) {
			portamento_max = portamento_voice;
		}
	}

	portamento_frequency = portamento_max.frequency;
}

void OscillatorComponent::render(Synth const & synth) {
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

	portamento.render();

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
	release.render();
}
