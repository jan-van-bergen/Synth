#include "filter.h"

#include "util.h"

void FilterComponent::update(Synth const & synth) {
	dsp::VAFilterMode mode;
	switch (filter_type) {
		case 0: mode = dsp::VAFilterMode::LOW_PASS;  break;
		case 1: mode = dsp::VAFilterMode::HIGH_PASS; break;
		case 2: mode = dsp::VAFilterMode::BAND_PASS; break;
		case 3: mode = dsp::VAFilterMode::OFF;       break;

		default: abort();
	}

	filter.set(mode, cutoff, resonance);

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_sample(i);
		sample = filter.process(sample);
		outputs[0].set_sample(i, sample);
	}
}

void FilterComponent::render(Synth const & synth) {
	if (ImGui::BeginCombo("Type", filter_names[filter_type])) {
		for (int j = 0; j < util::array_count(filter_names); j++) {
			if (ImGui::Selectable(filter_names[j], filter_type == j)) {
				filter_type = j;
			}
		}

		ImGui::EndCombo();
	}

	cutoff   .render(); ImGui::SameLine();
	resonance.render();
}

void FilterComponent::serialize_custom(json::Writer & writer) const {
	writer.write("filter_type", filter_type);
}

void FilterComponent::deserialize_custom(json::Object const & object) {
	filter_type = object.find_int("filter_type", 0);
}
