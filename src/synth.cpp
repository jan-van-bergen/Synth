#include "synth.h"

#include <queue>
#include <unordered_set>

#include <ImGui/imgui.h>

static constexpr auto CONNECTOR_SIZE = 16.0f;

void Synth::update(Sample buf[BLOCK_SIZE]) {
	for (auto component = update_list_begin; component < update_list_end; component++) {
		for (auto & output : (*component)->outputs) {
			output.clear();
		}

		(*component)->update(*this);
	}

	time += BLOCK_SIZE;

	// Collect the resulting audio samples
	memset(buf, 0, BLOCK_SIZE * sizeof(Sample));

	for (auto const & speaker : speakers) {
		for (auto const & input : speaker->inputs) {
			for (int i = 0; i < BLOCK_SIZE; i++) {
				buf[i] += input.get_sample(i);
			}
		}
	}

	for (int i = 0; i < BLOCK_SIZE; i++) buf[i] *= settings.master_volume;

	note_events.clear();
}

void Synth::render() {
	connections.clear();
	drag_handled = false;
	
	auto show_popup_open = ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) && ImGui::IsKeyPressed(SDL_SCANCODE_O);
	auto show_popup_save = ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) && ImGui::IsKeyPressed(SDL_SCANCODE_S);
	
	// Draw menu bar
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "Ctrl+O")) show_popup_open = true;
			if (ImGui::MenuItem("Save", "Ctrl+S")) show_popup_save = true;

			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Components")) {
			if (ImGui::MenuItem("Keyboard"))   add_component<KeyboardComponent>();
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

			if (ImGui::MenuItem("Filter"))      add_component<FilterComponent>();
			if (ImGui::MenuItem("Delay"))       add_component<DelayComponent>();
			if (ImGui::MenuItem("Distortion"))  add_component<DistortionComponent>();
			if (ImGui::MenuItem("Bit Crusher")) add_component<BitCrusherComponent>();
			if (ImGui::MenuItem("Compressor"))  add_component<CompressorComponent>();
			
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

		if (just_loaded) ImGui::SetNextWindowPos(ImVec2(component->pos[0], component->pos[1])); // Move Window to correct position if we just loaded a file

		ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(INFINITY, INFINITY));

		if (ImGui::Begin(label, &open, ImGuiWindowFlags_NoSavedSettings)) {
			component->render(*this);

			for (auto & input  : component->inputs)  render_connector_in (input);
			for (auto & output : component->outputs) render_connector_out(output);

			collapsed = false;

			auto window_pos = ImGui::GetWindowPos();
			component->pos[0] = window_pos.x;
			component->pos[1] = window_pos.y;
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
		auto draw_list = ImGui::GetBackgroundDrawList();

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
	
	auto const CONNECTION_COLOUR_SELECTED = ImColor(255, 100, 100);
	auto const CONNECTION_COLOUR_MIDI     = ImColor(100, 200, 100);
	auto const CONNECTION_COLOUR_AUDIO    = ImColor(200, 200, 100);

	// Draw Hermite Splite between Connectors
	for (auto const & connection : connections) {
		auto spline_start = ImVec2(connection.first ->pos[0], connection.first ->pos[1]);
		auto spline_end   = ImVec2(connection.second->pos[0], connection.second->pos[1]);

		auto selected = selected_connection.has_value() &&
			selected_connection.value().first  == connection.first &&
			selected_connection.value().second == connection.second;

		ImColor colour;
		if (selected) {
			colour = CONNECTION_COLOUR_SELECTED;
		} else if (connection.first->is_midi) {
			colour = CONNECTION_COLOUR_MIDI;	
		} else {
			colour = CONNECTION_COLOUR_AUDIO;
		}

		auto intersects = draw_connection(spline_start, spline_end, colour);

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

		settings.tempo        .render();
		settings.master_volume.render();
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

		draw_connection(spline_start, spline_end, dragging->is_midi ? CONNECTION_COLOUR_MIDI : CONNECTION_COLOUR_AUDIO);
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

		// Remove from Speaker list, if it was in there
		auto speaker = std::find(speakers.begin(), speakers.end(), component_to_be_removed);
		if (speaker != speakers.end()) {
			speakers.erase(speaker);
		}

		// Remove Component
		components.erase(std::find_if(components.begin(), components.end(), [component_to_be_removed](auto const & component) { return component.get() == component_to_be_removed; }));

		reconstruct_update_graph();
	}
	
	if (show_popup_open) file_dialog.show(false);
	if (show_popup_save) file_dialog.show(true);

	just_loaded = false;

	if (file_dialog.render()) {
		auto filename = file_dialog.selected_path.string();

		if (file_dialog.saving) {
			save_file(filename.c_str());
		} else {
			open_file(filename.c_str());
		}
	}

	// Debug tool to terminate infinite notes
	if (ImGui::IsKeyPressed(SDL_SCANCODE_F5)) {
		for (auto const & component : components) {
			auto osc = dynamic_cast<OscillatorComponent *>(component.get());

			if (osc) osc->voices.clear();
		}
	}
}

void Synth::connect(ConnectorOut & out, ConnectorIn & in, float weight) {
	if (out.component == in.component) return;

	if (out.is_midi != in.is_midi) return;

	out.others.push_back(&in);
	in .others.push_back(std::make_pair(&out, weight));

	reconstruct_update_graph();
}

void Synth::disconnect(ConnectorOut & out, ConnectorIn & in) {
	assert(out.is_midi == in.is_midi);

	out.others.erase(std::find   (out.others.begin(), out.others.end(), &in));
	in .others.erase(std::find_if(in .others.begin(), in .others.end(), [&out](auto pair) {
		return pair.first == &out;	
	}));

	reconstruct_update_graph();
}

void Synth::reconstruct_update_graph() {
	// Update list is constructed in reverse order
	update_list.resize(components.size());
	update_list_end   = update_list.data() + update_list.size();
	update_list_begin = update_list_end;

	std::queue<Component *> queue;

	// Find the Components with no outgoing connetions and push them to the Queue
	for (auto const & component : components) {
		auto no_outputs = true;

		for (auto const & output : component->outputs) {
			if (output.others.size() > 0) {
				no_outputs = false;
				break;
			}
		}

		if (no_outputs) queue.push(component.get());
	}

	std::unordered_map<Component *, int> num_outputs_satisfied;

	// Breadth First Search in reverse, starting at the outputs
	while (!queue.empty()) {
		auto component = queue.front();
		queue.pop();

		*(--update_list_begin) = component;

		for (auto const & input : component->inputs) {
			for (auto const & [other, weight] : input.others) {
				auto num_satisfied = ++num_outputs_satisfied[other->component];

				auto num_required = 0;
				for (auto const & output : other->component->outputs) {
					num_required += output.others.size();
				}

				if (num_satisfied == num_required) { // If all outputs are now satisfied, push onto queue to explore the Node
					queue.push(other->component);
				}

				assert(num_satisfied <= num_required);
			}
		}
	}
}

void Synth::render_connector_in(ConnectorIn & in) {
	auto label = in.name.c_str();
	
	auto pos = ImGui::GetCursorScreenPos();
	
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

	auto avail = ImGui::GetContentRegionAvail();

	ImGui::SameLine();
	ImGui::Text(label);

//	for (auto other : in.others) assert(std::find(other->others.begin(), other->others.end(), &in) != other->others.end());

	in.pos[0] = pos.x;
	in.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;
}

void Synth::render_connector_out(ConnectorOut & out) {
	auto label = out.name.c_str();
	auto size = ImGui::CalcTextSize(label);

	auto avail = ImGui::GetContentRegionAvail();

	ImGui::NewLine();
	ImGui::SameLine(avail.x - size.x - 8);
	ImGui::Text(label);
	ImGui::SameLine();
	
	auto pos = ImGui::GetCursorScreenPos();
	
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

	out.pos[0] = pos.x + 8.0f;
	out.pos[1] = pos.y + 0.5f * CONNECTOR_SIZE;
}

void Synth::open_file(char const * filename) {
	auto parser = json::Parser(filename);

	components.clear();
	speakers.clear();

	Param::links.clear();

	time = 0;
			
	settings.tempo         = 130;
	settings.master_volume = 1.0f;

	if (parser.root->type == json::JSON::Type::OBJECT) {
		auto json = static_cast<json::Object const *>(parser.root.get()); 

		for (auto const & object : json->attributes) {
			assert(object->type == json::JSON::Type::OBJECT);
			auto obj = static_cast<json::Object const *>(object.get());

			if (obj->name == "Settings") {
				settings.tempo         = obj->find<json::ValueInt   const>("tempo")        ->value;
				settings.master_volume = obj->find<json::ValueFloat const>("master_volume")->value;
			} else if (obj->name == "Connection") {
				auto id_out     = obj->find<json::ValueInt   const>("component_out")->value;
				auto id_in      = obj->find<json::ValueInt   const>("component_in") ->value;
				auto offset_out = obj->find<json::ValueInt   const>("offset_out")   ->value;
				auto offset_in  = obj->find<json::ValueInt   const>("offset_in")    ->value;
				auto weight     = obj->find<json::ValueFloat const>("weight")       ->value;

				// Find Components by ID
				auto component_out = std::find_if(components.begin(), components.end(), [id_out](auto const & component) { return component->id == id_out; });
				auto component_in  = std::find_if(components.begin(), components.end(), [id_in] (auto const & component) { return component->id == id_in;  });

				if (component_out == components.end() || component_in == components.end()) {
					printf("WARNING: Failed to load connection %i <-> %i!\n", id_out, id_in);
					continue;
				}

				auto & connector_out = (*component_out)->outputs[offset_out];
				auto & connector_in  = (*component_in) ->inputs [offset_in];

				connect(connector_out, connector_in, weight);
			} else {
				auto id    = obj->find<json::ValueInt   const>("id")   ->value;
				auto pos_x = obj->find<json::ValueFloat const>("pos_x")->value;
				auto pos_y = obj->find<json::ValueFloat const>("pos_y")->value;

				Component * component = nullptr;

						if (obj->name == "BitCrusherComponent") component = add_component<BitCrusherComponent>(id);
				else if (obj->name == "CompressorComponent") component = add_component<CompressorComponent>(id);
				else if (obj->name == "DecibelComponent")    component = add_component<DecibelComponent>(id);
				else if (obj->name == "DelayComponent")      component = add_component<DelayComponent>(id);
				else if (obj->name == "DistortionComponent") component = add_component<DistortionComponent>(id);
				else if (obj->name == "FilterComponent")     component = add_component<FilterComponent>(id);
				else if (obj->name == "OscillatorComponent") component = add_component<OscillatorComponent>(id);
				else if (obj->name == "PanComponent")        component = add_component<PanComponent>(id);
				else if (obj->name == "PianoRollComponent")  component = add_component<PianoRollComponent>(id);
				else if (obj->name == "SamplerComponent")    component = add_component<SamplerComponent>(id);
				else if (obj->name == "SequencerComponent")  component = add_component<SequencerComponent>(id);
				else if (obj->name == "SpeakerComponent")    component = add_component<SpeakerComponent>(id);
				else if (obj->name == "SpectrumComponent")   component = add_component<SpectrumComponent>(id);
				else if (obj->name == "SplitComponent")      component = add_component<SplitComponent>(id);
				else if (obj->name == "WaveTableComponent")  component = add_component<WaveTableComponent>(id);

				if (!component) {
					printf("WARNING: Unsupported Component '%s'!\n", obj->name.c_str());
					continue;
				}

				component->deserialize(*obj);

				component->pos[0] = pos_x;
				component->pos[1] = pos_y;
			}
		}
	}
	reconstruct_update_graph();

	unique_component_id = components.size();

	just_loaded = true;
}

void Synth::save_file(char const * filename) const {
	auto writer = json::Writer(filename);

	writer.object_begin("Settings");
	writer.write("tempo",         settings.tempo);
	writer.write("master_volume", settings.master_volume);
	writer.object_end();

	for (auto const & component : components) {
		writer.object_begin(util::get_type_name(*component.get()));
		writer.write("id",    component->id);
		writer.write("pos_x", component->pos[0]);
		writer.write("pos_y", component->pos[1]);

		component->serialize(writer);
				
		writer.object_end();
	}

	for (auto const & connection : connections) {
		float * connection_weight = nullptr;

		for (auto & [other, weight] : connection.second->others) {
			if (other == connection.first) {
				connection_weight = &weight;
				break;
			}
		}

		assert(connection_weight);

		// Unique IDs that identify the Components
		auto component_out = connection.first ->component->id;
		auto component_in  = connection.second->component->id;

		// Unique offsets that identify the Connectors
		auto offset_out = int(connection.first  - connection.first ->component->outputs.data());
		auto offset_in  = int(connection.second - connection.second->component->inputs .data());

		writer.object_begin("Connection");
		writer.write("component_out", component_out);
		writer.write("component_in",  component_in);
		writer.write("offset_out", offset_out);
		writer.write("offset_in",  offset_in);
		writer.write("weight", *connection_weight);
		writer.object_end();
	}
}
