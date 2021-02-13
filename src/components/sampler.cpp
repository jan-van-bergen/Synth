#include "components.h"

#include "synth.h"

void SamplerComponent::update(Synth const & synth) {
	auto note_events = inputs[0].get_events();

	for (auto const & note_event : note_events) {
		if (note_event.pressed) {
			auto time_offset = note_event.time - synth.time;

			auto frequency_note = util::note_freq(note_event.note);
			auto frequency_base = util::note_freq(base_note);

			step = frequency_note / frequency_base;

			current_sample = -step * time_offset;

			velocity = note_event.velocity;
		}
	}

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample_index = util::round(current_sample);

		if (0 <= sample_index && sample_index < samples.size()) {
			outputs[0].set_sample(i, velocity * samples[sample_index]);
		}

		current_sample += step;
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
