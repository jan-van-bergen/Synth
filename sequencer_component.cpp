#include "components.h"

#include "synth.h"

void SequencerComponent::update(Synth const & synth) {
	outputs[0].clear();

	for (int i = 0; i < BLOCK_SIZE; i++) {
		static constexpr auto SIXTEENTH_NOTE = 115 * SAMPLE_RATE / 1000;

		auto time = (synth.time + i) % (SIXTEENTH_NOTE * TRACK_SIZE);

		auto step = time / SIXTEENTH_NOTE;
		auto hit  = time % SIXTEENTH_NOTE == 0;

		if (hit) outputs[0].values[i] = steps[step];
	}
}

void SequencerComponent::render(Synth const & synth) {
	char label[32];

	for (int i = 0; i < TRACK_SIZE; i++) {
		if ((i / 4) % 2 == 0) {
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor(250, 100, 100, 100).Value);
		} else {
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor(250, 250, 250, 100).Value);
		}
		
		sprintf_s(label, "##%i", i);

		ImGui::VSliderFloat(label, ImVec2(16, 64), &steps[i], 0.0f, 1.0f);
		ImGui::SameLine();
		
		ImGui::PopStyleColor();
	}
}
