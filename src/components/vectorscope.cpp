#include "components.h"

#include <ImGui/implot.h>

void VectorscopeComponent::update(Synth const & synth) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		samples[sample_offset] = inputs[0].get_sample(i);
		sample_offset = util::wrap(sample_offset + 1, NUM_SAMPLES);
	}
}

void VectorscopeComponent::render(Synth const & synth) {
	auto average = Sample(0.0f);
	for (auto const & sample : samples) average += sample;
	average /= NUM_SAMPLES;
	
	auto variance   = Sample(0.0f);
	auto covariance = 0.0f;

	for (auto const & sample : samples) {
		auto left  = sample.left  - average.left;
		auto right = sample.right - average.right;

		variance   += Sample(left * left, right * right);
		covariance += left * right;
	}

	static_assert(NUM_SAMPLES > 1);
	variance   /= NUM_SAMPLES - 1.0f;
	covariance /= NUM_SAMPLES - 1.0f;

	float correlation;
	if (std::abs(variance.left) < 0.0001f || std::abs(variance.right) < 0.0001f) {
		correlation = 0.0f;
	} else {
		correlation = covariance / std::sqrt(variance.left * variance.right);
	}

	ImGui::Text("Correlation: %f", correlation);

	auto avail = ImGui::GetContentRegionAvail();
	auto space = ImVec2(avail.x, std::max(
		ImGui::GetTextLineHeightWithSpacing(),
		avail.y - (inputs.size() + outputs.size()) * ImGui::GetTextLineHeightWithSpacing()
	));

	ImPlot::SetNextPlotLimits(-1.0, 1.0, -1.0, 1.0, ImGuiCond_Always);
	
	if (ImPlot::BeginPlot("Vectorscope", "Left", "Right", space, ImPlotFlags_CanvasOnly)) {
		auto getter = [](void * data, int index) -> ImPlotPoint {
			auto const & sample = reinterpret_cast<Sample const *>(data)[index];
			return { sample.left, sample.right };
		};
		ImPlot::PlotScatterG("", getter, samples, NUM_SAMPLES);

		ImPlot::EndPlot();
	}
}
