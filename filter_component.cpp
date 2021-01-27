#include "components.h"

#include "util.h"

void FilterComponent::update(Synth const & synth) {
	auto g = std::tan(PI * cutoff * SAMPLE_RATE_INV); // Gain
	auto R = 1.0f - resonance;                        // Damping
    
	auto denom_inv = 1.0f / (1.0f + (2.0f * R * g) + g * g);

	outputs[0].clear();

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);

		auto high_pass = (sample - (2.0f * R + g) * state_1 - state_2) * denom_inv;
		auto band_pass = high_pass * g + state_1;
		auto  low_pass = band_pass * g + state_2;
	
		state_1 = g * high_pass + band_pass;
		state_2 = g * band_pass + low_pass;

		if (filter_type == 0) {
			outputs[0].values[i] = low_pass;
		} else if (filter_type == 1) {
			outputs[0].values[i] = high_pass;
		} else if (filter_type == 2) {
			outputs[0].values[i] = band_pass;
		} else {
			outputs[0].values[i] = sample; // Unfiltered
		}
	}
}

void FilterComponent::render(Synth const & synth) {
	if (ImGui::BeginCombo("Type", filter_names[filter_type])) {
		for (int j = 0; j < util::array_element_count(filter_names); j++) {
			if (ImGui::Selectable(filter_names[j], filter_type == j)) {
				filter_type = j;
			}
		}

		ImGui::EndCombo();
	}

	ImGui::SliderFloat("Cutoff",    &cutoff,   20.0f, 10000.0f);
	ImGui::SliderFloat("Resonance", &resonance, 0.0f, 1.0f);
}
