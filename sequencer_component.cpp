#include "components.h"

#include "synth.h"

void SequencerComponent::update(Synth const & synth) {
	auto sixteenth_note = size_t(60) * SAMPLE_RATE / (4 * synth.tempo);
	
	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto time = (synth.time + i) % (sixteenth_note * TRACK_SIZE);

		auto step = time / sixteenth_note;
		auto hit  = time % sixteenth_note == 0;

		if (hit) {
			outputs[0].values[i] = steps[step];

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

		ImGui::VSliderFloat(label, ImVec2(16, 64), &steps[i], 0.0f, 1.0f, "");
		ImGui::SameLine();
		
		ImGui::PopStyleColor();
	}
}
