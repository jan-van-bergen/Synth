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

	auto sample_length = float(samples.size());

	for (int v = 0; v < voices.size(); v++) {
		auto & voice = voices[v];

		for (int i = 0; i < BLOCK_SIZE; i++) {
			if (voice.current_sample >= 0.0f) {
				if (voice.current_sample >= sample_length) {
					// Voice is done playing, remove
					voices.erase(voices.begin() + v);
					v--;

					break;
				}

				outputs[0].get_sample(i) += voice.velocity * util::sample_linear(samples.data(), samples.size(), voice.current_sample);
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
