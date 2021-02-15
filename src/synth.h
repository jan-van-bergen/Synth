#pragma once
#include <set>

#include "components.h"

#include "file_dialog.h"

struct Synth {
	std::vector<std::unique_ptr<Component>> components;
	std::vector<SpeakerComponent *> speakers;

	int unique_component_id = 0;

	std::vector<Component *> update_list; // Underlying data, update list grows back to front because it is constructed in reverse order
	Component             ** update_list_begin = nullptr;
	Component             ** update_list_end   = nullptr;

	struct {
		Parameter<int> tempo = { "Tempo", 130, std::make_pair(60, 200), { 80, 110, 128, 140, 150, 174 } };

		Parameter<float> master_volume = { "Master Volume", 1.0f, std::make_pair(0.0f, 2.0f), { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f } };
	} settings;

	int time = 0;
	
	std::set<NoteEvent, NoteEvent::Compare> mutable note_events; // Note Events, seperate for each channel
	
	FileDialog file_dialog;
	bool just_loaded = false;

	Synth() {
		open_file("projects/default.json");
	}

	template<IsComponent T>
	T * add_component(int id = -1) {
		static_assert(meta::TypeListContains<AllComponents, T>::value); // Ensure we only instantiate known Components

		if (id == -1) id = unique_component_id++;

		auto component = std::make_unique<T>(id);

		if constexpr (std::is_same<T, SpeakerComponent>()) {
			speakers.push_back(component.get());
		}

		auto result = static_cast<T *>(components.emplace_back(std::move(component)).get());

		reconstruct_update_graph();

		return result;
	}

	void update(Sample buf[BLOCK_SIZE]);
	void render();
	
	bool    connect(ConnectorOut & out, ConnectorIn & in, float weight = 1.0f);
	void disconnect(ConnectorOut & out, ConnectorIn & in);

	void note_press(int note, float velocity, int time_offset = 0) const {
		note_events.insert(NoteEvent { true, time + time_offset, note, velocity });
	}

	void note_release(int note, int time_offset = 0) const {
		note_events.insert(NoteEvent { false, time + time_offset, note });
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
	struct Connection {
		ConnectorOut * out;
		ConnectorIn  * in;
		float        * weight;
	};

	std::vector<Connection> connections;
	std::optional<Connection> selected_connection;

	Connector * dragging = nullptr;
	bool        drag_handled;

	void reconstruct_update_graph();

	void render_connector_in (ConnectorIn  & in);
	void render_connector_out(ConnectorOut & out);

	void open_file(char const * filename);
	void save_file(char const * filename) const;
};
