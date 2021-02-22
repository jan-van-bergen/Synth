#include "components.h"

#include "synth.h"

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
	
	if (ImGui::Button("Load")) samples = util::load_wav(filename);

	base_note.render(util::note_name);
	
	auto getter = [](void * data, int index) -> float {
		auto const & sample = reinterpret_cast<Sample const *>(data)[index];
		return 0.5f * (sample.left + sample.right);
	};

	auto avail = ImGui::GetContentRegionAvail();
	auto space = ImVec2(avail.x, std::max(
		ImGui::GetTextLineHeightWithSpacing(), 
		avail.y - 2.0f * ImGui::GetTextLineHeightWithSpacing()
	));
	
	ImGui::PlotLines("Sample", getter, samples.data(), samples.size(), 0, nullptr, FLT_MAX, FLT_MAX, space);

	/*
	auto avail = ImGui::GetContentRegionAvail();
	auto space = ImVec2(avail.x, std::max(
		ImGui::GetTextLineHeightWithSpacing(), 
		avail.y - 2.0f * ImGui::GetTextLineHeightWithSpacing()
	));
	
	auto num_samples = samples.size();
	auto downsampled = std::make_unique<float[]>(num_samples);

	for (int i = 0; i < num_samples; i++) {
		downsampled[i] = samples[i].left;
	}

	while (num_samples > space.x) {
		num_samples /= 2;

		for (int i = 0; i < num_samples; i++) {
			downsampled[i] = 0.5f * (downsampled[2*i] + downsampled[2*i + 1]);
		}
	}

	ImGui::PlotLines("Sample", downsampled.get(), num_samples, 0, nullptr, FLT_MAX, FLT_MAX, space);
	*/
}

void SamplerComponent::serialize_custom(json::Writer & writer) const {
	writer.write("filename", filename);
}

void SamplerComponent::deserialize_custom(json::Object const & object) {
	auto const & found = object.find_string("filename", DEFAULT_FILENAME);

	strcpy_s(filename, found.c_str());
	samples = util::load_wav(filename);
}
