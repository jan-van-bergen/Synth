#include "sampler.h"

#include <ImGui/implot.h>

#include "synth/synth.h"

#include "util/scope_timer.h"

void SamplerComponent::load(char const * file) {
	filename = file;

	auto idx = filename.find_last_of("/\\");
	filename_display = filename.c_str() + (idx == std::string::npos ? 0 : idx + 1);

	samples = util::load_wav(file);

	if (samples.size() == 0) {
		visual.samples = { 0.0f };
		return;
	}

	visual.samples.resize(samples.size());

	for (int i = 0; i < samples.size(); i++) visual.samples[i] = 0.5f * (samples[i].left + samples[i].right);

	auto num_samples = samples.size();

	while (num_samples > VISUAL_NUM_SAMPLES) {
		for (int i = 0; i < num_samples; i += 2) {
			auto first  = visual.samples[i];
			auto second = visual.samples[i + 1];

			auto diff = std::abs(first - second);

			if (diff > 0.2f) {
				// Choose the sample with largest absolute amplitude
				if (std::abs(first) > std::abs(second)) {
					visual.samples[i/2] = first;
				} else {
					visual.samples[i/2] = second;
				}
			} else {
				// Average the two samples
				visual.samples[i/2] = 0.5f * (first + second);
			}
		}		

		num_samples /= 2;
	}

	visual.samples.resize(num_samples);

	visual.max_y = 0.001f;

	for (int i = 0; i < visual.samples.size(); i++) {
		visual.max_y = std::max(visual.max_y, std::abs(visual.samples[i]));
	}
}

void SamplerComponent::update(Synth const & synth) {
	auto steps_per_second = 4.0f / 60.0f * float(synth.settings.tempo);

	update_voices(steps_per_second);

	auto sample_length = float(samples.size());

	for (int v = 0; v < voices.size(); v++) {
		auto & voice = voices[v];

		auto frequency_note = util::note_freq(voice.note);
		auto frequency_base = util::note_freq(base_note);
			
		auto step = frequency_note / frequency_base;

		for (int i = voice.get_first_sample(synth.time); i < BLOCK_SIZE; i++) {
			auto time_in_seconds = voice.sample * SAMPLE_RATE_INV;
			auto time_in_steps   = time_in_seconds * steps_per_second;
		
			float amplitude;
			auto done = voice.apply_envelope(time_in_steps, attack, hold, decay, sustain, release, amplitude);
			
			if (done || voice.sample >= sample_length) {
				voices.erase(voices.begin() + v);
				v--;

				break;
			}

			outputs[0].get_sample(i) += amplitude * util::sample_linear(samples.data(), samples.size(), voice.sample);

			voice.sample += step;
		}
	}
}

void SamplerComponent::render(Synth const & synth) {
	ImGui::Text("%s", filename_display);
	ImGui::SameLine();
	
	if (ImGui::Button("Load")) {
		synth.file_dialog.show(FileDialog::Type::OPEN, "Open WAV File", "samples", ".wav", [this](char const * path) { load(path); });
	}
	
	attack .render(); ImGui::SameLine();
	hold   .render(); ImGui::SameLine();
	decay  .render(); ImGui::SameLine();
	sustain.render(); ImGui::SameLine();
	release.render();

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
			auto t = voice.sample * ratio;
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
