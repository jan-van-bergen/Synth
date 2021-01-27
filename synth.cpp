#include "synth.h"

#include <queue>
#include <unordered_set>

#include <ImGui/imgui.h>

static constexpr auto CONNECTOR_SIZE = 16.0f;

void Synth::update(Sample buf[BLOCK_SIZE]) {
	// Clear output of all components
	for (auto const & component : components) {
		for (auto & output : component->outputs) {
			output.clear();
		}
	}

	// Traverse the Graph formed by the connections
	std::queue<Component *> queue;
	for (auto source : sources) queue.push(source);

	std::unordered_set<Component *>      seen;
	std::unordered_map<Component *, int> num_inputs_satisfied;

	while (!queue.empty()) {
		auto component = queue.front();
		queue.pop();

		seen.insert(component);

		component->update(*this);

		for (auto & output : component->outputs) {
			for (auto other : output.others) {
				if (!seen.contains(other->component)) {
					auto inputs = ++num_inputs_satisfied[other->component];

					if (inputs == other->others.size()) {
						queue.push(other->component);
					}
				}
			}
		}
	}
	
	time += BLOCK_SIZE;

	// Collect the resulting audio samples
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

	// Draw menu bar
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::EndMenu();
		}
			
		if (ImGui::BeginMenu("Edit")) {
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Components")) {
			if (ImGui::MenuItem("Sequencer")) add_component<SequencerComponent>();

			if (ImGui::MenuItem("Oscilator")) add_component<OscilatorComponent>();
			if (ImGui::MenuItem("Sampler"))   add_component<SamplerComponent>();
			
			if (ImGui::MenuItem("Distortion")) add_component<DistortionComponent>();    
			if (ImGui::MenuItem("Delay"))      add_component<DelayComponent>();    
			if (ImGui::MenuItem("Filter"))     add_component<FilterComponent>();   
			
			if (ImGui::MenuItem("Speaker"))  add_component<SpeakerComponent>();  
			if (ImGui::MenuItem("Recorder")) add_component<RecorderComponent>();  

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	char label[32];

	// Draw Components
	for (int i = 0; i < components.size(); i++) {
		auto & component = components[i];

		sprintf_s(label, "%s##%i", component->name.c_str(), i);

		auto hidden = true;

		if (ImGui::Begin(label, nullptr, ImGuiWindowFlags_NoSavedSettings)) {
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

	auto draw_connection = [](ImVec2 spline_start, ImVec2 spline_end, ImColor colour) {
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
				h1 * spline_start.x + h2 * spline_end.x + h3 * t1.x + h4 * t2.x, 
				h1 * spline_start.y + h2 * spline_end.y + h3 * t1.y + h4 * t2.y
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
		auto spline_start = ImVec2(connection.first ->pos[0], connection.first ->pos[1]);
		auto spline_end   = ImVec2(connection.second->pos[0], connection.second->pos[1]);

		auto selected = selected_connection.has_value() &&
			selected_connection.value().first  == connection.first &&
			selected_connection.value().second == connection.second;

		auto intersects = draw_connection(spline_start, spline_end, selected ? ImColor(255, 100, 100) : ImColor(200, 200, 100));

		if (intersects && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			selected_connection = connection;
		}
		
		static constexpr auto width = 64.0f;

		ImGui::SetNextWindowPos (ImVec2(spline_start.x + 0.5f * (spline_end.x - spline_start.x), spline_start.y + 0.5f * (spline_end.y - spline_start.y)));
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

	if (ImGui::Begin("Manager")) {
		ImGui::SliderInt("Tempo", &tempo, 60, 200);
	}
	ImGui::End();

	auto const & io = ImGui::GetIO();

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) dragging = nullptr;

	if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
		if (selected_connection.has_value()) {
			disconnect(*selected_connection.value().first, *selected_connection.value().second);

			selected_connection = { };
		}
	}

	if (dragging) {
		auto spline_start = ImVec2(dragging->pos[0], dragging->pos[1]);
		auto spline_end   = io.MousePos;

		if (dragging->is_input) std::swap(spline_start, spline_end); // Always draw from out to in

		draw_connection(spline_start, spline_end, ImColor(200, 200, 100));
	}
}

void Synth::render_connector_in(ConnectorIn & in) {
	auto label = in.name.c_str();

	auto pos  = ImGui::GetCursorScreenPos();
	auto size = ImGui::CalcTextSize(label);
	
	ImGui::SmallButton("");

	auto min = ImGui::GetItemRectMin();
	auto max = ImGui::GetItemRectMax();

	if (ImGui::IsMouseHoveringRect(min, max)) {
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			if (!dragging) dragging = &in;
		} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if (dragging && dragging != &in) {
				if (!dragging->is_input) {
					connect(*reinterpret_cast<ConnectorOut *>(dragging), in);
				}

				dragging = nullptr;
			}
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
	
	ImGui::SmallButton("");
	
	auto min = ImGui::GetItemRectMin();
	auto max = ImGui::GetItemRectMax();

	if (ImGui::IsMouseHoveringRect(min, max)) {
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			if (!dragging) dragging = &out;
		} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if (dragging && dragging != &out) {
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
