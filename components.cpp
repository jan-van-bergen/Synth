#include "components.h"

#include <queue>
#include <unordered_set>

#include <ImGui/imgui.h>

#include "util.h"

Sample ConnectorIn::get_value(int i) const {
	if (other) {
		return other->values[i];
	} else {
		return 0.0f;
	}
}

void OscilatorComponent::update(Synth const & synth) {
	auto envelope = [](int time) -> float {
		static constexpr float attack  = 0.1f;
		static constexpr float hold    = 0.5f;
		static constexpr float decay   = 1.0f;
		static constexpr float sustain = 0.5f;
//		static constexpr float release = 1.0f;

		auto t = float(time) * SAMPLE_RATE_INV;

		if (t < attack) {
			return t / attack;
		}
		t -= attack;

		if (t < hold) {
			return 1.0f;
		}
		t -= hold;

		if (t < decay) {
			return util::lerp(1.0f, sustain, t / decay);
		}

		return sustain;
	};

	outputs[0].clear();

	for (auto const & [id, note] : synth.notes) {
		
		for (int i = 0; i < BLOCK_SIZE; i++) {
			auto time = synth.time + i;

			auto duration = time - note.time;

			auto t = float(time) * SAMPLE_RATE_INV;

			switch (waveform_index) {
				case 0: outputs[0].values[i] += util::generate_sine    (t, util::note_freq(note.note), note.velocity * 20.0f * envelope(duration)); break;
				case 1: outputs[0].values[i] += util::generate_square  (t, util::note_freq(note.note), note.velocity * 20.0f * envelope(duration)); break;
				case 2: outputs[0].values[i] += util::generate_triangle(t, util::note_freq(note.note), note.velocity * 20.0f * envelope(duration)); break;
				case 3: outputs[0].values[i] += util::generate_saw     (t, util::note_freq(note.note), note.velocity * 20.0f * envelope(duration)); break;

				default: abort();
			}
		}
	}
}

void OscilatorComponent::render(Synth const & synth) {
	if (ImGui::BeginCombo("Waveform", waveform_name)) {
		for (int j = 0; j < IM_ARRAYSIZE(OscilatorComponent::options); j++) {
			if (ImGui::Selectable(OscilatorComponent::options[j], waveform_index == j)) {
				waveform_name  = OscilatorComponent::options[j];
				waveform_index = j;
			}
		}

		ImGui::EndCombo();
	}
}

void FilterComponent::update(Synth const & synth) {
	auto g = std::tan(PI * cutoff * SAMPLE_RATE_INV); // Gain
	auto R = 1.0f - resonance;                        // Damping
    
	outputs[0].clear();

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);

		auto high_pass = (sample - (2.0f * R + g) * state_1 - state_2) / (1.0f + (2.0f * R * g) + g * g);
		auto band_pass = high_pass * g + state_1;
		auto  low_pass = band_pass * g + state_2;
	
		state_1 = g * high_pass + band_pass;
		state_2 = g * band_pass + low_pass;

		outputs[0].values[i] = low_pass;
	}
}

void FilterComponent::render(Synth const & synth) {
	ImGui::SliderFloat("Cutoff", &cutoff, 20.0f, 10000.0f);
	ImGui::SliderFloat("Resonance", &resonance, 0.0f, 1.0f);
}

void DelayComponent::update(Synth const & synth) {
	outputs[0].clear();

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_value(i);

		sample = sample + feedback * history[offset];
		history[offset] = sample;
	
		offset = (offset + 1) % HISTORY_SIZE;

		outputs[0].values[i] = sample;
	}
}

void DelayComponent::render(Synth const & synth) {
	ImGui::SliderFloat("Feedback", &feedback, 0.0f, 1.0f);
}

void SpeakerComponent::update(Synth const & synth) {
	
}

void SpeakerComponent::render(Synth const & synth) {
	ImGui::Text("Speaker");
}

void Synth::update(Sample buf[BLOCK_SIZE]) {
	std::queue<Component *> queue;
	for (auto source : sources) queue.push(source);

	std::unordered_set<Component *> seen;

	while (!queue.empty()) {
		auto component = queue.front();
		queue.pop();

		seen.insert(component);

		component->update(*this);

		for (auto & output : component->outputs) {
			if (output.other && !seen.contains(output.other->component)) {
				queue.push(output.other->component);
			}
		}
	}
	
	time += BLOCK_SIZE;

	for (auto const & sink : sinks) {
		for (auto const & input : sink->inputs) {
			if (input.other) {
				for (int i = 0; i < BLOCK_SIZE; i++) {
					buf[i] = input.other->values[i];
				}
			}
		}
	}
}

void Synth::render() {
	connections.clear();

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {		
			ImGui::EndMenu();
		}
			
		if (ImGui::BeginMenu("Edit")) {		
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Components")) {		
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	for (int i = 0; i < components.size(); i++) {
		auto & component = components[i];

//		ImGui::PushID(i);

		if (ImGui::Begin(component->name.c_str())) {
			component->render(*this);

			for (auto & input  : component->inputs)  render_connector_in (input);
			for (auto & output : component->outputs) render_connector_out(output);
		}
		ImGui::End();

//		ImGui::PopID();
	}

	auto draw_list = ImGui::GetForegroundDrawList();
	
	// Draw Hermite Splite between Connectors
	for (auto const & connection : connections) {
		auto p1 = ImVec2(connection.in .pos[0], connection.in .pos[1]);
		auto p2 = ImVec2(connection.out.pos[0], connection.out.pos[1]);

		auto t1 = ImVec2(-200.0f, 0.0f);
		auto t2 = ImVec2(-200.0f, 0.0f);

		static constexpr auto NUM_STEPS = 20;

		for (int s = 0; s <= NUM_STEPS; s++) {
			float t = float(s) / float(NUM_STEPS);

			float h1 = +2.0f * t * t * t - 3.0f * t * t + 1.0f;
			float h2 = -2.0f * t * t * t + 3.0f * t * t;
			float h3 =         t * t * t - 2.0f * t * t + t;
			float h4 =         t * t * t -        t * t;

			draw_list->PathLineTo(ImVec2(
				h1 * p1.x + h2 * p2.x + h3 * t1.x + h4 * t2.x, 
				h1 * p1.y + h2 * p2.y + h3 * t1.y + h4 * t2.y
			));
		}

		draw_list->PathStroke(ImColor (200, 200, 100), false, 3.0f);
	}
}

static constexpr auto CONNECTOR_SIZE = 16.0f;

void Synth::render_connector_in(ConnectorIn & in) {
	auto label = in.name.c_str();

	auto pos  = ImGui::GetCursorScreenPos();
	auto size = ImGui::CalcTextSize(label);

	if (ImGui::SmallButton("")) {
				
	}

	ImGui::SameLine(ImGui::GetWindowWidth() - size.x - 20);
	ImGui::Text(label);

	if (in.other) {
		in.pos[0] = pos.x;
		in.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;

		connections.push_back({ in, *in.other });
	}
}

void Synth::render_connector_out(ConnectorOut & out) {
	auto label = out.name.c_str();

	ImGui::Text(label);
	ImGui::SameLine(ImGui::GetWindowWidth() - 20);
	
	auto pos  = ImGui::GetCursorScreenPos();
	auto size = ImGui::CalcTextSize(label);
	
	if (ImGui::SmallButton("")) {
				
	}

	if (out.other) {
		out.pos[0] = pos.x;
		out.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;

		connections.push_back({ *out.other, out });
	}
}
