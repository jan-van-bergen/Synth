#include "components.h"

#include <queue>
#include <unordered_set>

#include <ImGui/imgui.h>

#include "util.h"

static constexpr auto CONNECTOR_SIZE = 16.0f;

Sample ConnectorIn::get_value(int i) const {
	Sample sample = 0.0f;

	for (auto const & [other, weight] : others) sample += weight * other->values[i];
	
	return sample;
}

void OscilatorComponent::update(Synth const & synth) {
	auto envelope = [](int time, float attack, float hold, float decay, float sustain) -> float {
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
				case 0: outputs[0].values[i] += util::generate_sine    (t, util::note_freq(note.note + transpose), note.velocity * 20.0f * envelope(duration, env_attack, env_hold, env_decay, env_sustain)); break;
				case 1: outputs[0].values[i] += util::generate_square  (t, util::note_freq(note.note + transpose), note.velocity * 20.0f * envelope(duration, env_attack, env_hold, env_decay, env_sustain)); break;
				case 2: outputs[0].values[i] += util::generate_triangle(t, util::note_freq(note.note + transpose), note.velocity * 20.0f * envelope(duration, env_attack, env_hold, env_decay, env_sustain)); break;
				case 3: outputs[0].values[i] += util::generate_saw     (t, util::note_freq(note.note + transpose), note.velocity * 20.0f * envelope(duration, env_attack, env_hold, env_decay, env_sustain)); break;

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

	ImGui::DragInt("Transpose", &transpose, 1.0f / 12.0f);

	ImGui::SliderFloat("Attack",  &env_attack,  0.0f, 4.0f);
	ImGui::SliderFloat("Hold",    &env_hold,    0.0f, 4.0f);
	ImGui::SliderFloat("Decay",   &env_decay,   0.0f, 4.0f);
	ImGui::SliderFloat("Sustain", &env_sustain, 0.0f, 1.0f);
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
			for (auto other : output.others) {
				if (!seen.contains(other->component)) {
					queue.push(other->component);
				}
			}
		}
	}
	
	time += BLOCK_SIZE;

	memset(buf, 0, BLOCK_SIZE * sizeof(Sample));

	for (auto const & sink : sinks) {
		for (auto const & input : sink->inputs) {
			for (auto const & [other, weight] : input.others) {
				for (int i = 0; i < BLOCK_SIZE; i++) {
					buf[i] += weight * other->values[i];
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
			if (ImGui::MenuItem("Oscilator")) add_component<OscilatorComponent>();
			if (ImGui::MenuItem("Filter"))    add_component<FilterComponent>();   
			if (ImGui::MenuItem("Delay"))     add_component<DelayComponent>();    
			if (ImGui::MenuItem("Speaker"))   add_component<SpeakerComponent>();  

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
				input.pos[0] = pos.x;
				input.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;

				for (auto const & [other, weight] : input.others) connections.push_back({ other, &input });
			}
			for (auto & output : component->outputs) {
				output.pos[0] = pos.x;
				output.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;

				for (auto other : output.others) connections.push_back({ &output, other });
			}
		}
	}

	auto draw_connection = [](ImVec2 a, ImVec2 b, ImColor colour) {
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

	auto idx = 0;

	// Draw Hermite Splite between Connectors
	for (auto const & connection : connections) {
		auto a = ImVec2(connection.first ->pos[0], connection.first ->pos[1]);
		auto b = ImVec2(connection.second->pos[0], connection.second->pos[1]);

		auto selected = selected_connection.has_value() &&
			selected_connection.value().first  == connection.first &&
			selected_connection.value().second == connection.second;

		auto intersects = draw_connection(a, b, selected ? ImColor(255, 100, 100) : ImColor(200, 200, 100));

		if (intersects && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			selected_connection = connection;
		}
		
		static constexpr auto width = 64.0f;

		ImGui::SetNextWindowPos (ImVec2(a.x + 0.5f * (b.x - a.x), a.y + 0.5f * (b.y - a.y)));
		ImGui::SetNextWindowSize(ImVec2(width, 16));
		
		float * connection_weight = nullptr;

		for (auto & [other, weight] : connection.second->others) {
			if (other == connection.first) {
				connection_weight = &weight;
				break;
			}
		}

		char test[32]; sprintf_s(test, "Connection##%i", idx++);
		ImGui::Begin(test, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
		ImGui::SliderFloat("", connection_weight, 0.0f, 1.0f);
		ImGui::End();
	}

	auto const & io = ImGui::GetIO();

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) dragging = nullptr;

	if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
		if (selected_connection.has_value()) {
			disconnect(*selected_connection.value().first, *selected_connection.value().second);

			selected_connection = { };
		}
	}

	if (dragging) {
		draw_connection(ImVec2(dragging->pos[0], dragging->pos[1]), io.MousePos, ImColor(200, 200, 100));
	}
}

void Synth::render_connector_in(ConnectorIn & in) {
	auto label = in.name.c_str();

	auto pos  = ImGui::GetCursorScreenPos();
	auto size = ImGui::CalcTextSize(label);

	if (ImGui::SmallButton("")) {
		if (dragging) {
			if (!dragging->is_input) {
				connect(*reinterpret_cast<ConnectorOut *>(dragging), in);
			}

			dragging = nullptr;
		} else {
			dragging = &in;
			drag_handled = true;
		}
	}

	ImGui::SameLine(ImGui::GetWindowWidth() - size.x - 20);
	ImGui::Text(label);

//	for (auto other : in.others) assert(std::find(other->others.begin(), other->others.end(), &in) != other->others.end());

	in.pos[0] = pos.x;
	in.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;
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

	ImGui::SmallButton("");

	if (ImGui::IsItemHovered()) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (!dragging) dragging = &out;
		} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if (dragging != &out) {
				if (dragging->is_input) {
					connect(out, *reinterpret_cast<ConnectorIn *>(dragging));
				}

				dragging = nullptr;
			}
		}
	}

	for (auto other : out.others) {
//		assert(std::find(other->others.begin(), other->others.end(), &out) != other->others.end());

		connections.push_back({ &out, other });
	}

	out.pos[0] = pos.x;
	out.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;
}
