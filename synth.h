#pragma once
#include "components.h"

#include "file_dialog.h"

struct Synth {
	std::vector<Component *> sources;
	std::vector<Component *> sinks;

	std::vector<std::unique_ptr<Component>> components;
	
	int unique_component_id = 0;

	std::vector<Component *> update_graph; // Order in which Components are updated, should be reconstructed if topology of the graph changes

	struct {
		Parameter<int> tempo = { "Tempo", 130, std::make_pair(60, 200), { 80, 110, 128, 140, 150, 174 } };

		Parameter<float> master_volume = { "Master Volume", 1.0f, std::make_pair(0.0f, 2.0f), { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f } };
	} settings;

	int time = 0;
	
	struct Note {
		int   note;
		float velocity;
		int   time;
	};

	mutable std::vector<Note> notes; // Currently held down notes
	
	FileDialog file_dialog;
	bool just_loaded = false;

	template<typename T> requires std::derived_from<T, Component>
	T * add_component(int id = -1) {
		if (id == -1) id = unique_component_id++;

		auto component = std::make_unique<T>(id);

		if (component->type == Component::Type::SOURCE) sources.push_back(component.get());
		if (component->type == Component::Type::SINK)   sinks  .push_back(component.get());
		
		auto result = static_cast<T *>(components.emplace_back(std::move(component)).get());

		reconstruct_update_graph();

		return result;
	}

	void update(Sample buf[BLOCK_SIZE]);
	void render();
	
	void    connect(ConnectorOut & out, ConnectorIn & in, float weight = 1.0f);
	void disconnect(ConnectorOut & out, ConnectorIn & in);

	void note_press(int note, float velocity, int time_offset = 0) const {
		if (std::find_if(notes.begin(), notes.end(), [note](auto const & n) { return n.note == note; }) == notes.end()) {
			notes.emplace_back(note, velocity, time + time_offset);
		}
	}

	void note_release(int note) const {
		auto n = std::find_if(notes.begin(), notes.end(), [note](auto const & n) { return n.note == note; });
		if (n != notes.end()) notes.erase(n);
	}

	void control_update(int control, float value) {
		auto & linked_params = Param::links[control];

		// If there is a Parameter waiting to link, link it to the current Controller
		if (Param::param_waiting_to_link) {
			linked_params.push_back(Param::param_waiting_to_link);

			Param::param_waiting_to_link->linked_controller = control;
			Param::param_waiting_to_link = nullptr;
		}

		value = util::clamp(value, 0.0f, 1.0f);

		// Update all Parameters that are linked to the current Controller
		for (auto param : linked_params) param->set_value(value);
	}

private:
	using Connection = std::pair<ConnectorOut *, ConnectorIn *>;

	std::vector<Connection> connections;
	std::optional<Connection> selected_connection;

	Connector * dragging = nullptr;
	bool        drag_handled;

	void reconstruct_update_graph();

	void render_connector_in (ConnectorIn  & in);
	void render_connector_out(ConnectorOut & out);
};
