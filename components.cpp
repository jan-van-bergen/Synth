#include "components.h"

#include <queue>
#include <unordered_set>

#include <ImGui/imgui.h>

#include "util.h"

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
		
	for (auto const & connection : connections) {
		draw_list->PathLineTo(ImVec2(connection.in .pos[0], connection.in .pos[1]));
		draw_list->PathLineTo(ImVec2(connection.out.pos[0], connection.out.pos[1]));
		draw_list->PathStroke(ImColor(200, 200, 100), false, 3.0f);
	}
}

/*
void Synth::render_oscilators() {
	
}

void Synth::render_speakers() {
	for (int i = 0; i < speakers.size(); i++) {
		auto & speaker = speakers[i];

//		ImGui::PushID(i);

		if (ImGui::Begin(i == 0 ? "Speaker##1" : "Speaker##2")) {
			for (auto & input  : speaker.inputs)  render_connector_in (input);
			for (auto & output : speaker.outputs) render_connector_out(output);
		}
		ImGui::End();

//		ImGui::PopID();
	}
}
*/

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
