#include "synth.h"

#include <queue>
#include <unordered_set>

#include <ImGui/imgui.h>

#include "knob.h"

void Synth::update(Sample buf[BLOCK_SIZE]) {
	for (auto const & component : components) {
		for (auto & output : component->outputs) {
			output.clear();
		}

		component->update(*this);
	}

	time += BLOCK_SIZE;

	// Collect the resulting audio samples
	std::memset(buf, 0, BLOCK_SIZE * sizeof(Sample));

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
	
	// If a Component window was closed, do the bookkeeping required to remove it
	if (component_to_be_removed) {
		// Disconnect inputs
		for (auto & input : component_to_be_removed->inputs) {
			if (dragging == &input) dragging = nullptr;

			while (!input.others.empty()) {
				auto & [other, weight] = input.others[0];
				disconnect(*other, input);
			}
		}

		// Disconnect outputs
		for (auto & output : component_to_be_removed->outputs) {
			if (dragging == &output) dragging = nullptr;

			while (!output.others.empty()) {
				auto & other = output.others[0];
				disconnect(output, *other);
			}
		}

		// Remove from Speaker list, if it was in there
		auto speaker = std::find(speakers.begin(), speakers.end(), component_to_be_removed);
		if (speaker != speakers.end()) {
			speakers.erase(speaker);
		}

		// Remove Component
		components.erase(std::find_if(components.begin(), components.end(), [rem = component_to_be_removed](auto const & component) { return component.get() == rem; }));

		compute_update_order();
	}

	file_dialog.render();

	if (ImGui::Begin("Settings")) {
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		settings.tempo        .render();
		settings.master_volume.render();
	}
	ImGui::End();

	// Debug utility to terminate infinite notes
	if (ImGui::IsKeyPressed(SDL_SCANCODE_F5)) {
		for (auto const & component : components) {
			auto osc = dynamic_cast<OscillatorComponent    *>(component.get()); if (osc) osc->clear();
			auto fm  = dynamic_cast<FMComponent            *>(component.get()); if (fm)  fm ->clear();
			auto add = dynamic_cast<AdditiveSynthComponent *>(component.get()); if (add) add->clear();
			auto smp = dynamic_cast<SamplerComponent       *>(component.get()); if (smp) smp->clear();
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

	compute_update_order();

	return true;
}

void Synth::disconnect(ConnectorOut & out, ConnectorIn & in) {
	assert(out.is_midi == in.is_midi);

	out.others.erase(std::find   (out.others.begin(), out.others.end(), &in));
	in .others.erase(std::find_if(in .others.begin(), in .others.end(), [&out](auto pair) {
		return pair.first == &out;	
	}));

	compute_update_order();
}

// Computes the order in which Components should be updated, based on the connections in the node graph
void Synth::compute_update_order() {
	std::queue<Component *> queue;

	// Find the Components with no outgoing connetions and push them to the Queue
	for (auto & component : components) {
		component->num_outputs_satisfied = 0;
		
		auto no_outputs = true;

		for (auto const & output : component->outputs) {
			if (output.others.size() > 0) {
				no_outputs = false;
				break;
			}
		}

		if (no_outputs) queue.push(component.get());
	}

	auto current_component_index = int(components.size());

	// Breadth First Search in reverse, starting at the outputs
	while (!queue.empty()) {
		auto component = queue.front();
		queue.pop();

		component->update_index = current_component_index--;

		for (auto const & input : component->inputs) {
			for (auto const & [other, weight] : input.others) {
				auto num_satisfied = ++other->component->num_outputs_satisfied;

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

	// Reorder according to update index
	std::sort(components.begin(), components.end(), [](auto const & a, auto const & b) -> bool {
		return a->update_index < b->update_index;
	});
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
			if (ImGui::MenuItem("Keyboard"))    add_component<KeyboardComponent>();
			if (ImGui::MenuItem("Sequencer"))   add_component<SequencerComponent>();
			if (ImGui::MenuItem("MIDI Player")) add_component<MIDIPlayerComponent>();
			if (ImGui::MenuItem("Improviser"))  add_component<ImproviserComponent>();
			if (ImGui::MenuItem("Arp"))         add_component<ArpComponent>();

			ImGui::Separator();

			if (ImGui::MenuItem("Oscillator")) add_component<OscillatorComponent>();
			if (ImGui::MenuItem("FM"))         add_component<FMComponent>();
			if (ImGui::MenuItem("Additive"))   add_component<AdditiveSynthComponent>();
			if (ImGui::MenuItem("Sampler"))    add_component<SamplerComponent>();
			
			ImGui::Separator();

			if (ImGui::MenuItem("Gain"))  add_component<GainComponent>();
			if (ImGui::MenuItem("Pan"))   add_component<PanComponent>();
			if (ImGui::MenuItem("Split")) add_component<SplitComponent>();
			
			ImGui::Separator();

			if (ImGui::MenuItem("Filter"))      add_component<FilterComponent>();
			if (ImGui::MenuItem("Delay"))       add_component<DelayComponent>();
			if (ImGui::MenuItem("Flanger"))     add_component<FlangerComponent>();
			if (ImGui::MenuItem("Phaser"))      add_component<PhaserComponent>();
			if (ImGui::MenuItem("Distortion"))  add_component<DistortionComponent>();
			if (ImGui::MenuItem("Bit Crusher")) add_component<BitCrusherComponent>();
			if (ImGui::MenuItem("Equalizer"))   add_component<EqualizerComponent>();
			if (ImGui::MenuItem("Compressor"))  add_component<CompressorComponent>();
			if (ImGui::MenuItem("Vocoder"))     add_component<VocoderComponent>();
			
			ImGui::Separator();

			if (ImGui::MenuItem("Speaker"))       add_component<SpeakerComponent>();
			if (ImGui::MenuItem("Spectrum"))      add_component<SpectrumComponent>();
			if (ImGui::MenuItem("Oscilloscope"))  add_component<OscilloscopeComponent>();
			if (ImGui::MenuItem("Decibel Meter")) add_component<DecibelComponent>();
			if (ImGui::MenuItem("Vectorscope"))   add_component<VectorscopeComponent>();
			
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (show_popup_open) file_dialog.show(FileDialog::Type::OPEN, "Open Project", "projects", ".json", [this](char const * path) { open_file(path); });
	if (show_popup_save) file_dialog.show(FileDialog::Type::SAVE, "Save Project", "projects", ".json", [this](char const * path) { save_file(path); });
}

void Synth::render_components() {
	char label[128] = { };

	component_to_be_removed = nullptr;

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

		ImGui::SetNextWindowSizeConstraints(ImVec2(64, 64), ImVec2(INFINITY, INFINITY));

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
	auto const CONNECTION_COLOUR_MIDI     = ImColor(100, 150, 100);
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

		auto const size_window = ImVec2(44, 44);
		auto const size_knob   = 32.0f;

		ImGui::SetNextWindowPos(ImVec2(pos.x - 0.5f * size_window.x, pos.y - 0.5f * size_window.y));
		ImGui::SetNextWindowSize(size_window);
		
		sprintf_s(label, "Connection##%i", idx++);
		
		ImGui::Begin(label, nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoFocusOnAppearing
		);
		ImGui::SetCursorPos(ImVec2(
			0.5f * (size_window.x - size_knob),
			0.5f * (size_window.y - size_knob)
		));
		ImGui::Knob(label, "", "", connection.weight, 0.0f, 1.0f, false, "%.1f", size_knob);
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
				settings.tempo        .deserialize(*obj);
				settings.master_volume.deserialize(*obj);
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
				auto component = try_add_component<AllComponents>(*this, obj->name, id);

				if (!component) {
					printf("WARNING: Unknown Component '%s'!\n", obj->name.c_str());
					continue;
				}

				component->deserialize(*obj);

				assert(!components_by_id.contains(id));
				components_by_id[id] = component;

				max_id = std::max(max_id, id);
			}
		}
	}

	compute_update_order();

	unique_component_id = max_id + 1;

	just_loaded = true;
}

void Synth::save_file(char const * filename) const {
	printf("Saving '%s'\n", filename);

	auto writer = json::Writer(filename);

	writer.object_begin("Settings");
	settings.tempo        .serialize(writer);
	settings.master_volume.serialize(writer);
	writer.object_end();

	// Serialize Components
	for (auto const & component : components) {
		writer.object_begin(util::get_type_name(*component.get()));
		
		component->serialize(writer);
				
		writer.object_end();
	}

	// Serialize Connections
	for (auto const & connection : connections) {
		// Unique IDs that identify the Components
		auto id_out = connection.out->component->id;
		auto id_in  = connection.in ->component->id;

		// Unique offsets that identify the Connectors
		auto offset_out = int(connection.out - connection.out->component->outputs.data());
		auto offset_in  = int(connection.in  - connection.in ->component->inputs .data());

		assert(0 <= offset_out && offset_out < connection.out->component->outputs.size());
		assert(0 <= offset_in  && offset_in  < connection.in ->component->inputs .size());

		writer.object_begin("Connection");
		writer.write("component_out", id_out);
		writer.write("component_in",  id_in);
		writer.write("offset_out", offset_out);
		writer.write("offset_in",  offset_in);
		writer.write("weight", *connection.weight);
		writer.object_end();
	}
}
