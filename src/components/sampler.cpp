#include "components.h"

void SamplerComponent::update(Synth const & synth) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		static constexpr auto EPSILON = 0.001f;

		auto abs = Sample::apply_function(std::fabsf, inputs[0].get_value(i));
		if (abs.left > EPSILON && abs.right > EPSILON) {
			velocity = abs.left;
			current_sample = 0; // Trigger sample on input
		}

		if (current_sample < samples.size()) {
			outputs[0].values[i] = velocity * samples[current_sample];
		}

		current_sample++;
	}
}

void SamplerComponent::render(Synth const & synth) {
	ImGui::InputText("File", filename, sizeof(filename));
	ImGui::SameLine();
	
	if (ImGui::Button("Load")) samples = util::load_wav(filename);
}

void SamplerComponent::serialize(json::Writer & writer) const {
	writer.write("filename", filename);
}

void SamplerComponent::deserialize(json::Object const & object) {
	strcpy_s(filename, object.find<json::ValueString const>("filename")->value.c_str());
	samples = util::load_wav(filename);
}
