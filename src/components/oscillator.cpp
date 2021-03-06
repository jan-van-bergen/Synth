#include "oscillator.h"

#include <ImGui/font_audio.h>

#include "synth/synth.h"
#include "util/util.h"

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

			Sample sample = { };

			// Generate selected waveform
			switch (waveform) {
				case 0: sample = sign * amplitude * generate_sine    (phase + voice.phase); break;
				case 1: sample = sign * amplitude * generate_triangle(phase + voice.phase); break;
				case 2: sample = sign * amplitude * generate_saw     (phase + voice.phase); break;
				case 3: sample = sign * amplitude * generate_square  (phase + voice.phase); break;
				case 4: sample = sign * amplitude * generate_square  (phase + voice.phase, 0.25f);  break;
				case 5: sample = sign * amplitude * generate_square  (phase + voice.phase, 0.125f); break;
				case 6: sample = sign * amplitude * generate_noise(seed); break;

				default: abort();
			}
			
			auto flt        = flt_amount * util::envelope(time_in_steps, flt_attack, flt_hold, flt_decay, flt_sustain);
			auto flt_cutoff = util::log_interpolate(20.f, 20000.0f, flt);

			voice.filter.set(dsp::VAFilterMode::LOW_PASS, flt_cutoff, flt_resonance);

			sample = voice.filter.process(sample);

			outputs[0].get_sample(i) += sample;
			
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
	auto fmt_waveform = [](int value, char * fmt, int len) -> void {
		switch (value) {
			case 0: strcpy_s(fmt, len, ICON_FAD_MODSINE);   break;
			case 1: strcpy_s(fmt, len, ICON_FAD_MODTRI);	break;
			case 2: strcpy_s(fmt, len, ICON_FAD_MODSAWUP);	break;
			case 3: strcpy_s(fmt, len, ICON_FAD_MODSQUARE);	break;
			case 4: strcpy_s(fmt, len, ICON_FAD_MODSQUARE);	break;
			case 5: strcpy_s(fmt, len, ICON_FAD_MODSQUARE);	break;
			case 6: strcpy_s(fmt, len, ICON_FAD_MODRANDOM);	break;

			default: abort();
		};
	};

	waveform.render(fmt_waveform); ImGui::SameLine();

	auto fmt_bool = [](int value, char * fmt, int len) -> void {
		strcpy_s(fmt, len, value ? "True" : "False");
	};

	invert.render(fmt_bool); ImGui::SameLine();
	phase .render();         ImGui::SameLine();

	transpose .render(); ImGui::SameLine();
	detune    .render(); ImGui::SameLine();
	portamento.render();

	if (ImGui::BeginTabBar("Envelopes")) {
		if (ImGui::BeginTabItem("Vol")) {
			attack .render(); ImGui::SameLine();
			hold   .render(); ImGui::SameLine();
			decay  .render(); ImGui::SameLine();
			sustain.render(); ImGui::SameLine();
			release.render();

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Flt")) {
			flt_attack .render(); ImGui::SameLine();
			flt_hold   .render(); ImGui::SameLine();
			flt_decay  .render(); ImGui::SameLine();
			flt_sustain.render(); ImGui::SameLine();

			flt_amount   .render(); ImGui::SameLine();
			flt_resonance.render();
			
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}
