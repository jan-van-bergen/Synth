#include "synth.h"

#include <queue>
#include <unordered_set>

#include <ImGui/imgui.h>

static constexpr auto CONNECTOR_SIZE = 16.0f;

void Synth::update(Sample buf[BLOCK_SIZE]) {
	for (auto component : update_graph) {
		for (auto & output : component->outputs) {
			output.clear();
		}

		component->update(*this);
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

	for (int i = 0; i < BLOCK_SIZE; i++) buf[i] *= master_volume;
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
			if (ImGui::MenuItem("Sequencer"))  add_component<SequencerComponent>();
			if (ImGui::MenuItem("Piano Roll")) add_component<PianoRollComponent>();

			ImGui::Separator();

			if (ImGui::MenuItem("Oscillator")) add_component<OscillatorComponent>();
			if (ImGui::MenuItem("Wave Table")) add_component<WaveTableComponent>();
			if (ImGui::MenuItem("Sampler"))    add_component<SamplerComponent>();
			
			ImGui::Separator();

			if (ImGui::MenuItem("Split")) add_component<SplitComponent>();
			if (ImGui::MenuItem("Pan"))   add_component<PanComponent>();
			
			ImGui::Separator();

			if (ImGui::MenuItem("Distortion")) add_component<DistortionComponent>();
			if (ImGui::MenuItem("Delay"))      add_component<DelayComponent>();
			if (ImGui::MenuItem("Filter"))     add_component<FilterComponent>();
			
			ImGui::Separator();

			if (ImGui::MenuItem("Speaker"))       add_component<SpeakerComponent>();
			if (ImGui::MenuItem("Spectrum"))      add_component<SpectrumComponent>();
			if (ImGui::MenuItem("Decibel Meter")) add_component<DecibelComponent>();
			
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	char label[128];

	Component * component_to_be_removed = nullptr;

	// Draw Components
	for (int i = 0; i < components.size(); i++) {
		auto component = components[i].get();

		sprintf_s(label, "%s##%p", component->name.c_str(), component);

		auto open      = true;
		auto collapsed = true;

		ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(INFINITY, INFINITY));

		if (ImGui::Begin(label, &open, ImGuiWindowFlags_NoSavedSettings)) {
			component->render(*this);

			for (auto & input  : component->inputs)  render_connector_in (input);
			for (auto & output : component->outputs) render_connector_out(output);

			collapsed = false;
		}

		auto pos = ImGui::GetCursorScreenPos();
		ImGui::End();

		if (!open) {
			component_to_be_removed = component;
		} else if (collapsed) { // If the Component's Window is collapsed, still draw its connections
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
		
		float * connection_weight = nullptr;

		for (auto & [other, weight] : connection.second->others) {
			if (other == connection.first) {
				connection_weight = &weight;
				break;
			}
		}

		auto pos = ImVec2(
			spline_start.x + 0.5f * (spline_end.x - spline_start.x), 
			spline_start.y + 0.5f * (spline_end.y - spline_start.y)
		);

		auto going_left = spline_end.x > spline_start.x;
		auto going_down = spline_end.y > spline_start.y;

		if (!(going_left ^ going_down)) pos.y -= 32.0f; // Move out of the way of the spline

		sprintf_s(label, "Connection##%i", idx++);
		
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(ImVec2(64, 16));
		
		ImGui::Begin(label, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
		ImGui::SliderFloat("", connection_weight, 0.0f, 1.0f, "%.1f");
		ImGui::End();
	}

	if (ImGui::Begin("Settings")) {
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		tempo.render();
		master_volume.render();
	}
	ImGui::End();

	auto const & io = ImGui::GetIO();

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
		dragging = nullptr;

		selected_connection = { };
	}

	if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
		if (selected_connection.has_value()) {
			disconnect(*selected_connection.value().first, *selected_connection.value().second);

			selected_connection = { };
		}
	}

	// If the user is dragging a new connection, draw it
	if (dragging) {
		auto spline_start = ImVec2(dragging->pos[0], dragging->pos[1]);
		auto spline_end   = io.MousePos;

		if (dragging->is_input) std::swap(spline_start, spline_end); // Always draw from out to in

		draw_connection(spline_start, spline_end, ImColor(200, 200, 100));
	}

	// If a Component window was closed, do the bookkeeping required to remove it
	if (component_to_be_removed) {
		// Disconnect inputs
		for (auto & input : component_to_be_removed->inputs) {
			for (auto & [other, weight] : input.others) {
				disconnect(*other, input);
			}
		}

		// Disconnect outputs
		for (auto & output : component_to_be_removed->outputs) {
			for (auto & other : output.others) {
				disconnect(output, *other);
			}
		}

		// Remove from source or sink lists
		if (component_to_be_removed->type == Component::Type::SOURCE) sources.erase(std::find(sources.begin(), sources.end(), component_to_be_removed));
		if (component_to_be_removed->type == Component::Type::SINK)   sinks  .erase(std::find(sinks  .begin(), sinks  .end(), component_to_be_removed));

		// Remove Component
		components.erase(std::find_if(components.begin(), components.end(), [component_to_be_removed](auto const & component) { return component.get() == component_to_be_removed; }));

		reconstruct_update_graph();
	}
}

void Synth::connect(ConnectorOut & out, ConnectorIn & in) {
	if (out.component == in.component) return;

	out.others.push_back(&in);
	in .others.push_back(std::make_pair(&out, 1.0f));

	reconstruct_update_graph();
}

void Synth::disconnect(ConnectorOut & out, ConnectorIn & in) {
	out.others.erase(std::find   (out.others.begin(), out.others.end(), &in));
	in .others.erase(std::find_if(in .others.begin(), in .others.end(), [&out](auto pair) {
		return pair.first == &out;	
	}));

	reconstruct_update_graph();
}

void Synth::reconstruct_update_graph() {
	update_graph.clear();

	// Traverse the Graph formed by the connections
	std::queue<Component *> queue;
	for (auto source : sources) queue.push(source);

	std::unordered_map<Component *, int> num_inputs_satisfied;

	while (!queue.empty()) {
		auto component = queue.front();
		queue.pop();

		update_graph.push_back(component);

		for (auto const & output : component->outputs) {
			for (auto other : output.others) {
				auto num_satisfied = ++num_inputs_satisfied[other->component];

				auto num_required = 0;
				for (auto const & input : other->component->inputs) {
					num_required += input.others.size();
				}

				if (num_satisfied == num_required) { // If all inputs are now satisfied, push onto queue to explore the Node
					queue.push(other->component);
				}

				assert(num_satisfied <= num_required);
			}
		}
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
