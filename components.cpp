#include "components.h"

#include <queue>
#include <unordered_set>

#include <ImGui/imgui.h>

#include "util.h"

static constexpr auto CONNECTOR_SIZE = 16.0f;

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
	const char * waveform_name = options[waveform_index];

	if (ImGui::BeginCombo("Waveform", waveform_name)) {
		for (int j = 0; j < IM_ARRAYSIZE(OscilatorComponent::options); j++) {
			if (ImGui::Selectable(OscilatorComponent::options[j], waveform_index == j)) {
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
	ImGui::SliderFloat("Cutoff",    &cutoff,   20.0f, 10000.0f);
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
	drag_handled = false;

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

	char label[32];

	for (int i = 0; i < components.size(); i++) {
		auto & component = components[i];

		sprintf_s(label, "%s##%i", component->name.c_str(), i);

		auto hidden = true;

		if (ImGui::Begin(label)) {
			component->render(*this);

			for (auto & input  : component->inputs)  render_connector_in (input);
			for (auto & output : component->outputs) render_connector_out(output);

			hidden = false;
		}
			auto pos = ImGui::GetCursorScreenPos();
		ImGui::End();

		if (hidden) {

			for (auto & input : component->inputs) {
				if (input.other) {
					input.pos[0] = pos.x;
					input.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;

					connections.push_back({ input.other, &input });
				}
			}
			for (auto & output : component->outputs) {
				if (output.other) {
					output.pos[0] = pos.x;
					output.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;

					connections.push_back({ &output, output.other });
				}
			}
		}
	}

	auto draw_hermite = [](ImVec2 a, ImVec2 b, ImColor colour) {
		auto draw_list = ImGui::GetForegroundDrawList();

		const auto t1 = ImVec2(200.0f, 0.0f);
		const auto t2 = ImVec2(200.0f, 0.0f);

		static constexpr auto NUM_STEPS = 100;
		static constexpr auto THICKNESS = 3.0f;

		bool intersects = false;

		auto const & io = ImGui::GetIO();

		for (int s = 0; s <= NUM_STEPS; s++) {
			auto t = float(s) / float(NUM_STEPS);

			auto h1 = +2.0f * t * t * t - 3.0f * t * t + 1.0f;
			auto h2 = -2.0f * t * t * t + 3.0f * t * t;
			auto h3 =         t * t * t - 2.0f * t * t + t;
			auto h4 =         t * t * t -        t * t;

			auto point = ImVec2(
				h1 * a.x + h2 * b.x + h3 * t1.x + h4 * t2.x, 
				h1 * a.y + h2 * b.y + h3 * t1.y + h4 * t2.y
			);

			draw_list->PathLineTo(point);

			auto diff = ImVec2(point.x - io.MousePos.x, point.y - io.MousePos.y);

			if (diff.x * diff.x + diff.y * diff.y < THICKNESS * THICKNESS) intersects = true;
		}

		draw_list->PathStroke(colour, false, THICKNESS);

		return intersects;
	};

	// Draw Hermite Splite between Connectors
	for (auto const & connection : connections) {
		auto p1 = ImVec2(connection.first ->pos[0], connection.first ->pos[1]);
		auto p2 = ImVec2(connection.second->pos[0], connection.second->pos[1]);

		auto selected = selected_connection.has_value() &&
			selected_connection.value().first  == connection.first &&
			selected_connection.value().second == connection.second;

		auto intersects = draw_hermite(p1, p2, selected ? ImColor(255, 100, 100) : ImColor(200, 200, 100));

		if (intersects && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			selected_connection = connection;
		}
	}

	auto const & io = ImGui::GetIO();

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) dragging = nullptr;

	if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
		if (selected_connection.has_value()) {
			selected_connection.value().first ->other = nullptr;
			selected_connection.value().second->other = nullptr;

			selected_connection = { };
		}
	}

	if (dragging) {
		draw_hermite(ImVec2(dragging->pos[0], dragging->pos[1]), io.MousePos, ImColor(200, 200, 100));
	}
}

void Synth::render_connector_in(ConnectorIn & in) {
	auto label = in.name.c_str();

	auto pos  = ImGui::GetCursorScreenPos();
	auto size = ImGui::CalcTextSize(label);

	if (ImGui::SmallButton("")) {
		if (dragging) {
			connect(*reinterpret_cast<ConnectorOut *>(dragging), in);

			dragging = nullptr;
		} else {
			dragging = &in;
			drag_handled = true;
		}
	}

	ImGui::SameLine(ImGui::GetWindowWidth() - size.x - 20);
	ImGui::Text(label);

	if (in.other) {
		assert(in.other->other == &in);

		in.pos[0] = pos.x;
		in.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;

//		connections.push_back({ in.other, &in });
	}
}

void Synth::render_connector_out(ConnectorOut & out) {
	auto label = out.name.c_str();

	ImGui::Text(label);
	ImGui::SameLine(ImGui::GetWindowWidth() - 20);
	
	auto pos  = ImGui::GetCursorScreenPos();
	auto size = ImGui::CalcTextSize(label);
	
	/*if (ImGui::SmallButton("")) {
		if (dragging) {
			connect(out, *reinterpret_cast<ConnectorIn *>(dragging));

			dragging = nullptr;
		} else {
			dragging = &out;
			drag_handled = true;
		}
	}*/

	if (ImGui::SmallButton("")) printf("bruh");

	if (ImGui::IsItemHovered()) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (!dragging) dragging = &out;
		} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if (dragging != &out) {
				connect(out, *reinterpret_cast<ConnectorIn *>(dragging));

				dragging = nullptr;
			}
		}
	}

	if (out.other) {
		assert(out.other->other == &out);

		out.pos[0] = pos.x;
		out.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;

		connections.push_back({ &out, out.other });
	}
}
