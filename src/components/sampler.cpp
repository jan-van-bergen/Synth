#include "components.h"

#include "synth.h"

void SamplerComponent::update(Synth const & synth) {
	auto note_events = inputs[0].get_events();

	for (auto const & note_event : note_events) {
		if (note_event.pressed) {
			auto time_offset = note_event.time - synth.time;

			auto frequency_note = util::note_freq(note_event.note);
			auto frequency_base = util::note_freq(base_note);
			
			auto step = frequency_note / frequency_base;
			auto current_sample = -step * time_offset;

			voices.emplace_back(current_sample, step, note_event.velocity);
		}
	}

	for (int v = 0; v < voices.size(); v++) {
		auto & voice = voices[v];

		for (int i = 0; i < BLOCK_SIZE; i++) {
			auto sample_index = util::round(voice.current_sample);

			if (sample_index >= 0) {
				if (sample_index < samples.size()) {
					outputs[0].get_sample(i) += voice.velocity * samples[sample_index];
				} else {
					// Voice is done playing, remove
					voices.erase(voices.begin() + v);
					v--;

					break;
				}
			}

			voice.current_sample += voice.step;
		}
	}
}

void SamplerComponent::render(Synth const & synth) {
	ImGui::InputText("File", filename, sizeof(filename));
	ImGui::SameLine();
	
	if (ImGui::Button("Load")) samples = util::load_wav(filename);

	base_note.render(util::note_name);
}

void SamplerComponent::serialize(json::Writer & writer) const {
	writer.write("filename", filename);
	writer.write("base_note", base_note);
}

void SamplerComponent::deserialize(json::Object const & object) {
	auto const & found = object.find_string("filename", DEFAULT_FILENAME);

	strcpy_s(filename, found.c_str());
	samples = util::load_wav(filename);

	base_note = object.find_int("base_note", base_note.default_value);
}
