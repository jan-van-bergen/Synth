#include "sequencer.h"

#include "synth.h"

static constexpr auto EPSILON = 0.001f;

void SequencerComponent::update(Synth const & synth) {
	auto sixteenth_note = size_t(60) * SAMPLE_RATE / (4 * synth.settings.tempo);
	
	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto time = (synth.time + i) % (sixteenth_note * TRACK_SIZE);

		auto step = time / sixteenth_note;
		auto hit  = time % sixteenth_note == 0;

		if (hit) {
			if (pattern[step] > EPSILON) {
				outputs[0].add_event(NoteEvent::make_release(synth.time + i, util::note<util::NoteName::C, 3>()));
				outputs[0].add_event(NoteEvent::make_press  (synth.time + i, util::note<util::NoteName::C, 3>(), pattern[step]));
			}

			current_step = step;
		}
	}
}

void SequencerComponent::render(Synth const & synth) {
	char label[32];

	for (int i = 0; i < TRACK_SIZE; i++) {
		// Draw quarter notes with alternating colours for clarity
		if ((i / 4) % 2 == 0) {
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor(250, 100, 100, current_step == i ? 150 : 100).Value);
		} else {
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor(250, 250, 250, current_step == i ? 150 : 100).Value);
		}
		
		sprintf_s(label, "##%i", i);

		ImGui::VSliderFloat(label, ImVec2(16, 64), &pattern[i], 0.0f, 1.0f, "");
		
		if (i != TRACK_SIZE - 1) ImGui::SameLine();
		
		ImGui::PopStyleColor();
	}
}

void SequencerComponent::serialize_custom(json::Writer & writer) const {
	writer.write("pattern", TRACK_SIZE, pattern);
}

void SequencerComponent::deserialize_custom(json::Object const & object) {
	object.find_array("pattern", TRACK_SIZE, pattern);
}
