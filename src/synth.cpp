#include "synth.h"

#include <queue>
#include <unordered_set>

#include <ImGui/imgui.h>

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
	
	render_menu();
	render_components();
	render_connections();
	
	if (file_dialog.render()) {
		auto filename = file_dialog.selected_path.string();

		if (file_dialog.saving) {
			save_file(filename.c_str());
		} else {
			open_file(filename.c_str());
		}
	}

	if (ImGui::Begin("Settings")) {
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		settings.tempo        .render();
		settings.master_volume.render();
	}
	ImGui::End();

	// Debug tool to terminate infinite notes
	if (ImGui::IsKeyPressed(SDL_SCANCODE_F5)) {
		for (auto const & component : components) {
			auto osc = dynamic_cast<OscillatorComponent *>(component.get());

			if (osc) osc->voices.clear();
		}
	}
}

bool Synth::connect(ConnectorOut & out, ConnectorIn & in, float weight) {
	if (out.component == in.component) return false;
	if (out.is_midi   != in.is_midi)   return false;

	for (auto other : out.others) {
		if (other == &in) return false; // These two Connectors are already connected
	}

	out.others.push_back(&in);
	in .others.push_back(std::make_pair(&out, weight));

	reconstruct_update_graph();

	return true;
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

void Synth::render_menu() {
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
			if (ImGui::MenuItem("Sampler"))    add_component<SamplerComponent>();
			
			ImGui::Separator();

			if (ImGui::MenuItem("Split")) add_component<SplitComponent>();
			if (ImGui::MenuItem("Pan"))   add_component<PanComponent>();
			
			ImGui::Separator();

			if (ImGui::MenuItem("Filter"))      add_component<FilterComponent>();
			if (ImGui::MenuItem("Delay"))       add_component<DelayComponent>();
			if (ImGui::MenuItem("Flanger"))     add_component<FlangerComponent>();
			if (ImGui::MenuItem("Distortion"))  add_component<DistortionComponent>();
			if (ImGui::MenuItem("Bit Crusher")) add_component<BitCrusherComponent>();
			if (ImGui::MenuItem("Compressor"))  add_component<CompressorComponent>();
			
			ImGui::Separator();

			if (ImGui::MenuItem("Speaker"))       add_component<SpeakerComponent>();
			if (ImGui::MenuItem("Spectrum"))      add_component<SpectrumComponent>();
			if (ImGui::MenuItem("Oscilloscope"))  add_component<OscilloscopeComponent>();
			if (ImGui::MenuItem("Decibel Meter")) add_component<DecibelComponent>();
			
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (show_popup_open) file_dialog.show(false);
	if (show_popup_save) file_dialog.show(true);
}

void Synth::render_components() {
	char label[128] = { };

	Component * component_to_be_removed = nullptr;

	// Draw Components
	for (int i = 0; i < components.size(); i++) {
		auto component = components[i].get();

		sprintf_s(label, "%s##%p", component->name.c_str(), component);

		auto open      = true;
		auto collapsed = true;

		if (just_loaded) { // Move and resize the Window if we just loaded a file
			ImGui::SetNextWindowPos(ImVec2(component->pos[0], component->pos[1]));

			if (component->size[0] > 0.0f && component->size[1] > 0.0f) {
				ImGui::SetNextWindowSize(ImVec2(component->size[0], component->size[1]));
			}
		}

		ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(INFINITY, INFINITY));

		if (ImGui::Begin(label, &open, ImGuiWindowFlags_NoSavedSettings)) {
			component->render(*this);

			for (auto & input  : component->inputs)  render_connector_in (input);
			for (auto & output : component->outputs) render_connector_out(output);

			collapsed = false;

			auto window_pos  = ImGui::GetWindowPos();
			auto window_size = ImGui::GetWindowSize();

			component->pos [0] = window_pos.x;
			component->pos [1] = window_pos.y;
			component->size[0] = window_size.x;
			component->size[1] = window_size.y;
		}

		auto pos = ImGui::GetCursorScreenPos();
		ImGui::End();

		if (!open) {
			component_to_be_removed = component;
		} else if (collapsed) { // If the Component's Window is collapsed, still draw its connections
			for (auto & input : component->inputs) {
				input.pos[0] = pos.x;
				input.pos[1] = pos.y + 0.5f * Connector::RENDER_SIZE;

				for (auto & [other, weight] : input.others) connections.emplace_back(other, &input, &weight);
			}

			for (auto & output : component->outputs) {
				output.pos[0] = pos.x;
				output.pos[1] = pos.y + 0.5f * Connector::RENDER_SIZE;
			}
		}
	}
	
	just_loaded = false;

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
}

void Synth::render_connections() {
	char label[128] = { };

	auto draw_connection = [](ImVec2 spline_start, ImVec2 spline_end, ImColor colour) {
		auto draw_list = ImGui::GetBackgroundDrawList();

		auto const t1 = ImVec2(200.0f, 0.0f);
		auto const t2 = ImVec2(200.0f, 0.0f);

		static constexpr auto NUM_STEPS = 100;
		static constexpr auto THICKNESS = 3.0f;

		auto intersects = false;

		auto const & io = ImGui::GetIO();

		auto point_prev = spline_start;

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

			// Calculate distance of mouse cursor to line segment
			auto pa = ImVec2(io.MousePos.x - point_prev.x, io.MousePos.y - point_prev.y);
			auto ba = ImVec2(point.x       - point_prev.x, point.y       - point_prev.y);

			auto h = util::clamp(
				(pa.x * ba.x + pa.y * ba.y) /
				(ba.x * ba.x + ba.y * ba.y)
			);

			auto v = ImVec2(pa.x - ba.x * h, pa.y - ba.y * h);

			auto dist_squared = v.x*v.x + v.y*v.y;
			if (dist_squared < THICKNESS * THICKNESS) intersects = true;

			point_prev = point;
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
		auto spline_start = ImVec2(connection.out->pos[0], connection.out->pos[1]);
		auto spline_end   = ImVec2(connection.in ->pos[0], connection.in ->pos[1]);

		auto selected = selected_connection.has_value() &&
			selected_connection.value().out == connection.out &&
			selected_connection.value().in  == connection.in;

		ImColor colour;
		if (selected) {
			colour = CONNECTION_COLOUR_SELECTED;
		} else if (connection.in->is_midi) {
			colour = CONNECTION_COLOUR_MIDI;	
		} else {
			colour = CONNECTION_COLOUR_AUDIO;
		}

		auto intersects = draw_connection(spline_start, spline_end, colour);

		if (intersects && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			selected_connection = connection;
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
		
		ImGui::Begin(label, nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar
		);
		ImGui::SliderFloat("", connection.weight, 0.0f, 1.0f, "%.1f");
		ImGui::End();
	}
	
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
		dragging = nullptr;

		selected_connection = { };
	}

	if (ImGui::IsKeyPressed(SDL_SCANCODE_DELETE)) {
		if (selected_connection.has_value()) {
			disconnect(*selected_connection.value().out, *selected_connection.value().in);

			selected_connection = { };
		}
	}

	// If the user is dragging a new connection, draw it
	if (dragging) {
		auto spline_start = ImVec2(dragging->pos[0], dragging->pos[1]);
		auto spline_end   = ImGui::GetIO().MousePos;

		if (dragging->is_input) std::swap(spline_start, spline_end); // Always draw from output to input

		auto colour = dragging->is_midi ? CONNECTION_COLOUR_MIDI : CONNECTION_COLOUR_AUDIO;
		
		draw_connection(spline_start, spline_end, colour);
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

	ImGui::SameLine();
	ImGui::Text(label);

	for (auto & [other, weight] : in.others) {
		connections.emplace_back(other, &in, &weight);
	}

	in.pos[0] = pos.x;
	in.pos[1] = pos.y + 0.5f * Connector::RENDER_SIZE;
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

	out.pos[0] = pos.x + 8.0f;
	out.pos[1] = pos.y + 0.5f * Connector::RENDER_SIZE;
}

template<typename ComponentList>
Component * try_add_component(Synth & synth, std::string const & name, int id) {
	using ComponentType = typename ComponentList::Head;

	if (name == util::get_type_name<ComponentType>()) {
		return synth.add_component<ComponentType>(id);
	}

	if constexpr (ComponentList::size > 1) {
		return try_add_component<ComponentList::Tail>(synth, name, id);
	} else {
		return nullptr;
	}
}

void Synth::open_file(char const * filename) {
	printf("Opening '%s'\n", filename);

	auto parser = json::Parser(filename);

	components.clear();
	speakers.clear();

	Param::links.clear();

	time = 0;
	
	settings.tempo         = settings.tempo        .default_value;
	settings.master_volume = settings.master_volume.default_value;
	
	auto max_id = -1;

	if (parser.root->type == json::JSON::Type::OBJECT) {
		auto json = static_cast<json::Object const *>(parser.root.get()); 

		std::unordered_map<int, Component *> components_by_id; // Used to find Component by its ID in O(1) time

		for (auto const & object : json->attributes) {
			assert(object->type == json::JSON::Type::OBJECT);
			auto obj = static_cast<json::Object const *>(object.get());

			if (obj->name == "Settings") {
				settings.tempo         = obj->find_int  ("tempo",         settings.tempo        .default_value);
				settings.master_volume = obj->find_float("master_volume", settings.master_volume.default_value);
			} else if (obj->name == "Connection") {
				auto id_out     = obj->find_int("component_out", -1);
				auto id_in      = obj->find_int("component_in",  -1);
				auto offset_out = obj->find_int("offset_out");
				auto offset_in  = obj->find_int("offset_in");
				auto weight     = obj->find_float("weight");

				auto component_out = components_by_id[id_out];
				auto component_in  = components_by_id[id_in];

				if (component_out == nullptr || component_in == nullptr) {
					printf("WARNING: Failed to load connection %i <-> %i!\n", id_out, id_in);
					continue;
				}

				auto & connector_out = component_out->outputs[offset_out];
				auto & connector_in  = component_in ->inputs [offset_in];

				auto valid = connect(connector_out, connector_in, weight);
				if (!valid) {
					printf("WARNING: Failed to connect %i <-> %i!\n", id_out, id_in);
					continue;
				}
			} else {
				auto obj_id = obj->find<json::ValueInt const>("id");
				if (!obj_id) {
					printf("WARNING: Component '%s' does not have an ID!\n", obj->name.c_str());
					continue;
				}

				auto id = obj_id->value;

				auto pos_x  = obj->find_float("pos_x", 100.0f);
				auto pos_y  = obj->find_float("pos_y", 100.0f);
				auto size_x = obj->find_float("size_x");
				auto size_y = obj->find_float("size_y");

				Component * component = try_add_component<AllComponents>(*this, obj->name, id);

				if (!component) {
					printf("WARNING: Unsupported Component '%s'!\n", obj->name.c_str());
					continue;
				}

				component->deserialize(*obj);

				component->pos [0] = pos_x;
				component->pos [1] = pos_y;
				component->size[0] = size_x;
				component->size[1] = size_y;

				assert(!components_by_id.contains(id));
				components_by_id[id] = component;

				max_id = std::max(max_id, id);
			}
		}
	}

	reconstruct_update_graph();

	unique_component_id = max_id + 1;

	just_loaded = true;
}

void Synth::save_file(char const * filename) const {
	printf("Saving '%s'\n", filename);

	auto writer = json::Writer(filename);

	writer.object_begin("Settings");
	writer.write("tempo",         settings.tempo);
	writer.write("master_volume", settings.master_volume);
	writer.object_end();

	// Serialize Components
	for (auto const & component : components) {
		writer.object_begin(util::get_type_name(*component.get()));
		writer.write("id",     component->id);
		writer.write("pos_x",  component->pos[0]);
		writer.write("pos_y",  component->pos[1]);
		writer.write("size_x", component->size[0]);
		writer.write("size_y", component->size[1]);

		component->serialize(writer);
				
		writer.object_end();
	}

	// Serialize Connections
	for (auto const & connection : connections) {
		// Unique IDs that identify the Components
		auto component_out = connection.out->component->id;
		auto component_in  = connection.in ->component->id;

		// Unique offsets that identify the Connectors
		auto offset_out = int(connection.out - connection.out->component->outputs.data());
		auto offset_in  = int(connection.in  - connection.in ->component->inputs .data());

		writer.object_begin("Connection");
		writer.write("component_out", component_out);
		writer.write("component_in",  component_in);
		writer.write("offset_out", offset_out);
		writer.write("offset_in",  offset_in);
		writer.write("weight", *connection.weight);
		writer.object_end();
	}
}
