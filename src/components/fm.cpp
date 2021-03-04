#include "components.h"

#include "synth.h"

void FMComponent::update(Synth const & synth) {
	auto steps_per_second = 4.0f / 60.0f * float(synth.settings.tempo);

	auto note_events = inputs[0].get_events();

	for (auto const & note_event : note_events) {
		if (note_event.pressed) {
			voices.emplace_back(note_event.note, note_event.velocity);
		} else {
			voices.erase(std::remove_if(voices.begin(), voices.end(), [note = note_event.note](auto voice) {
				return voice.note == note;
			}), voices.end());
		}
	}
	
	for (auto & voice : voices) {
		auto carrier_freq = util::note_freq(voice.note);
		
		for (int i = 0; i < BLOCK_SIZE; i++) {
			auto time_in_seconds = voice.sample * SAMPLE_RATE_INV;
			auto time_in_steps   = time_in_seconds * steps_per_second;

			auto phase = TWO_PI * time_in_seconds * carrier_freq; 

			Sample sample = { };

			float next_operator_values[NUM_OPERATORS] = { };

			for (int o = 0; o < NUM_OPERATORS; o++) {
				auto phs = ratios[o] * phase;

				// Modulate phase of operator o using the values of other operators p
				for (int p = 0; p < NUM_OPERATORS; p++) {
					phs += weights[p][o] * voice.operator_values[p];
				}

				auto value = util::envelope(time_in_steps, attacks[o], holds[o], decays[o], sustains[o]) * std::sin(phs);
				next_operator_values[o] = value;

				sample += outs[o] * value;
			}

			memcpy(voice.operator_values, next_operator_values, sizeof(voice.operator_values));

			outputs[0].get_sample(i) += voice.velocity * sample;

			voice.sample += 1.0f;
		}
	}
}

void FMComponent::render(Synth const & synth) {
	char label[128] = { };

	for (int op_to = 0; op_to < NUM_OPERATORS; op_to++) {
		for (int op_from = 0; op_from < NUM_OPERATORS; op_from++) {
			sprintf_s(label, "Operator %i to %i", op_from, op_to);

			ImGui::Knob(label, "", label, &weights[op_from][op_to], 0.0f, 2.0f, false, "", 12.0f);
			ImGui::SameLine();
		}

		sprintf_s(label, "Out %i", op_to);

		ImGui::Knob(label, "", label, &outs[op_to], 0.0f, 1.0f, false, "", 12.0f);
	}

	if (ImGui::BeginTabBar("Operators")) {
		for (int i = 0; i < NUM_OPERATORS; i++) {
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
}

void FMComponent::serialize_custom(json::Writer & writer) const {
	writer.write("weights", NUM_OPERATORS * NUM_OPERATORS, &weights[0][0]);
	writer.write("outs",    NUM_OPERATORS, outs);
}

void FMComponent::deserialize_custom(json::Object const & object) {
	object.find_array("weights", util::array_count(weights), &weights[0][0]);
	object.find_array("outs",    util::array_count(outs),    outs);
}
