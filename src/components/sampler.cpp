#include "components.h"

#include <ImGui/implot.h>

#include "synth.h"

#include "scope_timer.h"

void SamplerComponent::load(char const * file) {
	filename = file;

	auto idx = filename.find_last_of("/\\");
	filename_display = filename.c_str() + (idx == std::string::npos ? 0 : idx + 1);

	samples = util::load_wav(file);

	if (samples.size() == 0) {
		visual.samples = { 0.0f };
		return;
	}

	visual.samples.resize(VISUAL_NUM_SAMPLES);

	static constexpr auto FILTER_WIDTH = 3.0f;

	auto scale = float(samples.size()) / float(VISUAL_NUM_SAMPLES);
	auto filter_width = scale * FILTER_WIDTH;
	
	auto kernel = std::vector<float>(2 * int(ceilf(filter_width)) + 1);

	auto sum = 0.0f;

	// Sample kernel using box filter
	for (int i = 0; i < kernel.size(); i++) {
		auto sample = util::sample_box<float>(i - 0.5f * kernel.size(), 1.0f / scale, [](float x) -> float { return util::lanczos(x, FILTER_WIDTH); });

		kernel[i] = sample;
		sum      += sample;
	}

	// Normalize kernel
	for (int i = 0; i < kernel.size(); i++) kernel[i] /= sum;

	// Convolution
	for (int i = 0; i < visual.samples.size(); i++) {
		auto center = (float(i) + 0.5f) * scale;
		auto offset = int(floorf(center - filter_width));

		auto accum = 0.0f;

		for (int j = 0; j < kernel.size(); j++) {
			int index = util::clamp<int>(offset + j, 0, samples.size() - 1);

			accum += kernel[j] * 0.5f * (samples[index].left + samples[index].right);
		}

		visual.samples[i] = accum;
	}

	visual.max_y = 0.001f;

	for (int i = 0; i < visual.samples.size(); i++) {
		visual.max_y = std::max(visual.max_y, std::abs(visual.samples[i]));
	}
}

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
	ImGui::Text("%s", filename_display);
	ImGui::SameLine();
	
	if (ImGui::Button("Load")) {
		synth.file_dialog.show(FileDialog::Type::OPEN, "Open WAV File", "samples", [this](char const * path) { load(path); });
	}

	base_note.render(util::note_name);
	ImGui::SameLine();
	
	auto avail = ImGui::GetContentRegionAvail();
	auto space = ImVec2(avail.x, std::max(
		ImGui::GetTextLineHeightWithSpacing(),
		avail.y - (inputs.size() + outputs.size()) * ImGui::GetTextLineHeightWithSpacing()
	));

	ImPlot::SetNextPlotLimits(0.0, visual.samples.size(), -visual.max_y, visual.max_y, ImGuiCond_Always);
	
	ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0.0f, 0.0f));
	ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha,   0.25f);

	if (ImPlot::BeginPlot("Sample", nullptr, nullptr, space, ImPlotFlags_CanvasOnly, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations)) {
		ImPlot::PlotShaded("", visual.samples.data(), visual.samples.size());
		ImPlot::PlotLine  ("", visual.samples.data(), visual.samples.size());

		auto ratio = float(visual.samples.size()) / float(samples.size());

		for (auto const & voice : voices) {
			auto t = voice.current_sample * ratio;
			ImPlot::PlotVLines("", &t, 1);
		}

		ImPlot::EndPlot();
	}

	ImPlot::PopStyleVar();
	ImPlot::PopStyleVar();
}

void SamplerComponent::serialize_custom(json::Writer & writer) const {
	writer.write("filename", filename.c_str());
}

void SamplerComponent::deserialize_custom(json::Object const & object) {
	auto file = object.find_string("filename", DEFAULT_FILENAME);
	load(file.c_str());
}
