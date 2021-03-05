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

static Sample generate_noise(unsigned & seed) {
	return util::randf(seed) * 2.0f - 1.0f;
}

void OscillatorComponent::update(Synth const & synth) {
	auto steps_per_second = 4.0f / 60.0f * float(synth.settings.tempo);

	update_voices(steps_per_second);

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

		for (int i = voice.get_first_sample(synth.time); i < BLOCK_SIZE; i++) {
			auto time_in_seconds = voice.sample * SAMPLE_RATE_INV;
			auto time_in_steps   = time_in_seconds * steps_per_second;

			float amplitude;
			auto done = voice.apply_envelope(time_in_steps, attack, hold, decay, sustain, release, amplitude);
			
			if (done) {
				voices.erase(voices.begin() + v);
				v--;

				break;
			}

			// Generate selected waveform
			switch (waveform_index) {
				case 0: outputs[0].get_sample(i) += sign * amplitude * generate_sine    (phase + voice.phase); break;
				case 1: outputs[0].get_sample(i) += sign * amplitude * generate_triangle(phase + voice.phase); break;
				case 2: outputs[0].get_sample(i) += sign * amplitude * generate_saw     (phase + voice.phase); break;
				case 3: outputs[0].get_sample(i) += sign * amplitude * generate_square  (phase + voice.phase); break;
				case 4: outputs[0].get_sample(i) += sign * amplitude * generate_square  (phase + voice.phase, 0.25f);  break;
				case 5: outputs[0].get_sample(i) += sign * amplitude * generate_square  (phase + voice.phase, 0.125f); break;
				case 6: outputs[0].get_sample(i) += sign * amplitude * generate_noise(seed); break;

				default: abort();
			}
			
			float frequency;

			// Apply portamento
			if (time_in_steps < portamento) {
				frequency = util::lerp(portamento_frequency, note_frequency, time_in_steps / portamento);
			} else {
				frequency = note_frequency;
			}

			// Advance phase of the wave
			auto phase_delta = frequency * SAMPLE_RATE_INV;
			voice.phase = std::fmod(voice.phase + phase_delta, 1.0f);

			voice.sample += 1.0f;
			
			auto released = voice.release_time < time_in_steps;

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
	ImGui::PushItemWidth(-64.0f);

	if (ImGui::BeginCombo("Waveform", waveform_names[waveform_index])) {
		for (int i = 0; i < util::array_count(waveform_names); i++) {
			if (ImGui::Selectable(waveform_names[i], waveform_index == i)) {
				waveform_index = i;
			}
		}

		ImGui::EndCombo();
	}

	auto bool_to_str = [](int value, char * fmt, int len) -> void {
		strcpy_s(fmt, len, value ? "True" : "False");
	};

	invert.render(bool_to_str); ImGui::SameLine();
	phase .render();            ImGui::SameLine();

	transpose .render(); ImGui::SameLine();
	detune    .render(); ImGui::SameLine();
	portamento.render();

	attack .render(); ImGui::SameLine();
	hold   .render(); ImGui::SameLine();
	decay  .render(); ImGui::SameLine();
	sustain.render(); ImGui::SameLine();
	release.render();

	ImGui::PopItemWidth();
}

void OscillatorComponent::serialize_custom(json::Writer & writer) const {
	writer.write("waveform", waveform_index);
}

void OscillatorComponent::deserialize_custom(json::Object const & object) {
	waveform_index = object.find_int("waveform", 3);
}
