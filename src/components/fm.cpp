#include "fm.h"

#include "synth.h"

void FMComponent::update(Synth const & synth) {
	auto steps_per_second = 4.0f / 60.0f * float(synth.settings.tempo);

	update_voices(steps_per_second);

	for (int v = 0; v < voices.size(); v++) {
		auto & voice = voices[v];

		auto carrier_freq = util::note_freq(voice.note);
		
		for (int i = voice.get_first_sample(synth.time); i < BLOCK_SIZE; i++) {
			auto time_in_seconds = voice.sample * SAMPLE_RATE_INV;
			auto time_in_steps   = time_in_seconds * steps_per_second;
			
			float amplitude;
			auto done = voice.apply_envelope(time_in_steps, 0.0f, INFINITY, 0.0f, 1.0f, release, amplitude);
			
			if (done) {
				voices.erase(voices.begin() + v);
				v--;

				break;
			}

			Sample sample = { };

			float next_operator_values[FM_NUM_OPERATORS] = { };

			auto phase = TWO_PI * time_in_seconds * carrier_freq; 

			for (int o = 0; o < FM_NUM_OPERATORS; o++) {
				auto phs = ratios[o] * phase;

				// Modulate phase of operator o using the values of other operators p
				for (int p = 0; p < FM_NUM_OPERATORS; p++) {
					phs += weights[p][o] * voice.operator_values[p];
				}

				auto value = util::envelope(time_in_steps, attacks[o], holds[o], decays[o], sustains[o]) * std::sin(phs);
				next_operator_values[o] = value;

				sample += outs[o] * value;
			}

			std::memcpy(voice.operator_values, next_operator_values, sizeof(voice.operator_values));

			outputs[0].get_sample(i) += amplitude * sample;

			voice.sample += 1.0f;
		}
	}
}

void FMComponent::render(Synth const & synth) {
	char label[128] = { };

	ImVec2 cur = { };

	for (int op_to = 0; op_to < FM_NUM_OPERATORS; op_to++) {
		for (int op_from = 0; op_from < FM_NUM_OPERATORS; op_from++) {
			sprintf_s(label, "Operator %i to %i", op_from, op_to);
			ImGui::Knob(label, "", label, &weights[op_from][op_to], 0.0f, 2.0f, false, "", 12.0f);

			auto spacing = op_from < FM_NUM_OPERATORS - 1 ? -1.0f : 12.0f;
			ImGui::SameLine(0.0f, spacing);
		}

		sprintf_s(label, "Out %i", op_to);
		ImGui::Knob(label, "", label, &outs[op_to], 0.0f, 1.0f, false, "", 12.0f);

		if (op_to == 0) {
			// Save cursor position to allow drawing on the same line later on
			ImGui::SameLine();
			cur = ImGui::GetCursorPos();
			ImGui::NewLine();
		}
	}

	if (ImGui::BeginTabBar("Operators")) {
		for (int i = 0; i < FM_NUM_OPERATORS; i++) {
			sprintf_s(label, "Op %i", i);

			if (ImGui::BeginTabItem(label)) {
				ratios[i].render(); ImGui::SameLine();

				attacks [i].render(); ImGui::SameLine();
				holds   [i].render(); ImGui::SameLine();
				decays  [i].render(); ImGui::SameLine();
				sustains[i].render();

				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::SetCursorPos(cur);
	release.render();
}

void FMComponent::serialize_custom(json::Writer & writer) const {
	writer.write("weights", FM_NUM_OPERATORS * FM_NUM_OPERATORS, &weights[0][0]);
	writer.write("outs",    FM_NUM_OPERATORS, outs);
}

void FMComponent::deserialize_custom(json::Object const & object) {
	object.find_array("weights", FM_NUM_OPERATORS * FM_NUM_OPERATORS, &weights[0][0]);
	object.find_array("outs",    FM_NUM_OPERATORS, outs);
}
