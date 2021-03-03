#include "components.h"

#include <ImGui/implot.h>

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
	auto space = ImVec2(avail.x, std::max(
		ImGui::GetTextLineHeightWithSpacing(),
		avail.y - (inputs.size() + outputs.size()) * ImGui::GetTextLineHeightWithSpacing()
	));

	ImPlot::SetNextPlotLimits(0.0, BLOCK_SIZE, -1.0, 1.0, ImGuiCond_Always);
	
	if (ImPlot::BeginPlot("Osciloscope", nullptr, nullptr, space, ImPlotFlags_CanvasOnly, ImPlotAxisFlags_NoDecorations)) {
		ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

		ImPlot::PlotShaded("", samples, BLOCK_SIZE);
		ImPlot::PlotLine  ("", samples, BLOCK_SIZE);

		ImPlot::PopStyleVar();
		ImPlot::EndPlot();
	}
}
