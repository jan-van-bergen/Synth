#include "components.h"

void OscilloscopeComponent::update(Synth const & synth) {
	memset(samples, 0, sizeof(samples));

	for (auto const & [other, weight] : inputs[0].others) {
		for (int i = 0; i < BLOCK_SIZE; i++) {
			samples[i] += weight * other->get_sample(i).left;
		}
	}
}

void OscilloscopeComponent::render(Synth const & synth) {
	auto avail = ImGui::GetContentRegionAvail();
	ImGui::PlotLines("", samples, BLOCK_SIZE, 0, nullptr, -1.0f, 1.0f, ImVec2(avail.x, avail.y - ImGui::GetTextLineHeightWithSpacing()));
}
