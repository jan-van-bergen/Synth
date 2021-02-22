#include "components.h"

#include <ImGui/implot.h>

#include "synth.h"

#include "scope_timer.h"

void SamplerComponent::load() {
	samples = util::load_wav(filename);

	if (samples.size() == 0) {
		visual.samples = { 0.0f };
		return;
	}

	visual.samples.resize(VISUAL_NUM_SAMPLES);

#if 0
	// Copy samples
	for (int j = 0; j < num_samples; j++) samples[j] = 0.5f * (samples[j].left + samples[j].right);

	// Keep downsampling by a factor of 2 (box filter) until the sample is small enough
	while (num_samples > VISUAL_NUM_SAMPLES) {
		for (int j = 0; j < num_samples; j += 2) {
			samples[j / 2] = 0.5f * (samples[j] + samples[j + 1]);
		}

		num_samples /= 2;
	}

	samples.resize(num_samples);
#else
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
#endif
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
	ImGui::InputText("File", filename, sizeof(filename));
	ImGui::SameLine();
	
	if (ImGui::Button("Load")) load();

	base_note.render(util::note_name);
	
	auto avail = ImGui::GetContentRegionAvail();
	auto space = ImVec2(avail.x, std::max(
		ImGui::GetTextLineHeightWithSpacing(),
		avail.y - (inputs.size() + outputs.size()) * ImGui::GetTextLineHeightWithSpacing()
	));

#if 0
	auto getter = [](void * data, int index) -> float {
		auto const & sample = reinterpret_cast<Sample const *>(data)[index];
		return 0.5f * (sample.left + sample.right);
	};

	ImGui::PlotLines("Sample", getter, samples.data(), samples.size(), 0, nullptr, FLT_MAX, FLT_MAX, space);
#else
	ImPlot::SetNextPlotLimits(0.0, visual.samples.size(), -visual.max_y, visual.max_y, ImGuiCond_Always);
	
	ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0.0f, 0.0f));
	ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha,   0.25f);

	ImPlot::BeginPlot("Sample", nullptr, nullptr, space, ImPlotFlags_CanvasOnly, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
	ImPlot::PlotShaded("", visual.samples.data(), visual.samples.size());
	ImPlot::PlotLine  ("", visual.samples.data(), visual.samples.size());
	ImPlot::EndPlot();

	ImPlot::PopStyleVar();
	ImPlot::PopStyleVar();
#endif
}

void SamplerComponent::serialize_custom(json::Writer & writer) const {
	writer.write("filename", filename);
}

void SamplerComponent::deserialize_custom(json::Object const & object) {
	auto const & found = object.find_string("filename", DEFAULT_FILENAME);

	strcpy_s(filename, found.c_str());
	load();
}
