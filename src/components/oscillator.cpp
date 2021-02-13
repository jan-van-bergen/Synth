#include "components.h"

#include "synth.h"
#include "util.h"

static Sample generate_sine(float phase) {
	return std::sin(TWO_PI * phase);
}

static Sample generate_square(float phase, float pulse_width = 0.5f) {
	return std::fmodf(phase, 1.0f) < pulse_width ? 1.0f : -1.0f;
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
	auto steps_per_second = 4.0f / 60.0f * float(synth.settings.tempo);

	auto note_events = inputs[0].get_events();

	for (auto const & note_event : note_events) {
		if (note_event.pressed) {
			voices.emplace_back(note_event.note, note_event.velocity, note_event.time);
		} else {
			auto voice = std::find_if(voices.begin(), voices.end(), [note = note_event.note](auto voice) {
				return voice.note == note && std::isinf(voice.release_time);
			});

			if (voice != voices.end()) voice->release_time = (note_event.time - voice->start_time) * SAMPLE_RATE_INV * steps_per_second;
		}
	}

	auto sign = invert ? -1.0f : 1.0f;

	struct PortamentoState {
		int   index;
		float frequency;
	};

	PortamentoState portamento_max = { -1, portamento_frequency };

	for (int v = 0; v < voices.size(); v++) {
		auto & voice = voices[v];

		auto note_frequency = util::note_freq(voice.note + transpose) * std::pow(2.0f, detune / 1200.0f);

		PortamentoState portamento_voice = { 0, portamento_frequency };
		
		for (int i = 0; i < BLOCK_SIZE; i++) {
			if (synth.time + i < voice.start_time) continue;

			auto t     = voice.sample * SAMPLE_RATE_INV;
			auto steps = t * steps_per_second;

			// Apply envelope
			auto amplitude = voice.velocity * envelope(steps, attack, hold, decay, sustain);
			
			auto released = voice.release_time < float(synth.time + i - voice.start_time) * SAMPLE_RATE_INV * steps_per_second;

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
				case 0: outputs[0].get_sample(i) += sign * amplitude * generate_sine    (phase + voice.phase); break;
				case 1: outputs[0].get_sample(i) += sign * amplitude * generate_triangle(phase + voice.phase); break;
				case 2: outputs[0].get_sample(i) += sign * amplitude * generate_saw     (phase + voice.phase); break;
				case 3: outputs[0].get_sample(i) += sign * amplitude * generate_square  (phase + voice.phase); break;
				case 4: outputs[0].get_sample(i) += sign * amplitude * generate_square  (phase + voice.phase, 0.25f);  break;
				case 5: outputs[0].get_sample(i) += sign * amplitude * generate_square  (phase + voice.phase, 0.125f); break;
				case 6: outputs[0].get_sample(i) += sign * amplitude * generate_noise(); break;

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

	invert.render();
	phase .render();

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

void OscillatorComponent::serialize(json::Writer & writer) const {
	writer.write("waveform", waveform_index);
	writer.write("invert",   invert);
	writer.write("phase",    phase);

	writer.write("transpose",  transpose);
	writer.write("detune",     detune);
	writer.write("portamento", portamento);

	writer.write("attack",  attack);
	writer.write("hold",    hold);
	writer.write("decay",   decay);
	writer.write("sustain", sustain);
	writer.write("release", release);
}

void OscillatorComponent::deserialize(json::Object const & object) {
	waveform_index = object.find_int  ("waveform", 3);
	invert         = object.find_int  ("invert",   invert.default_value);
	phase          = object.find_float("phase",    phase .default_value);

	transpose  = object.find_int  ("transpose",  transpose .default_value);
	detune     = object.find_float("detune",     detune    .default_value);
	portamento = object.find_float("portamento", portamento.default_value);

	attack  = object.find_float("attack",  attack .default_value);
	hold    = object.find_float("hold",    hold   .default_value);
	decay   = object.find_float("decay",   decay  .default_value);
	sustain = object.find_float("sustain", sustain.default_value);
	release = object.find_float("release", release.default_value);
}
