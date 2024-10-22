#pragma once
#include <set>

#include "components/components.h"

#include "util/file_dialog.h"

struct Synth {
	std::vector<std::unique_ptr<Component>> components;
	std::vector<SpeakerComponent *> speakers;

	int unique_component_id = 0;

	struct {
		Parameter<int> tempo = { nullptr, "tempo", "Tmp", "Tempo", 130, std::make_pair(60, 200), { 80, 110, 128, 140, 150, 174 } };

		Parameter<float> master_volume = { nullptr, "master_volume", "Vol", "Master Volume", 1.0f, std::make_pair(0.0f, 2.0f), { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f } };
	} settings;

	int time = 0;
	
	std::set<NoteEvent, NoteEvent::Compare> note_events;
	
	FileDialog mutable file_dialog;
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

		auto success = compute_update_order();
		assert(success);

		return result;
	}

	void update(Sample buf[BLOCK_SIZE]);
	void render();
	
	bool    connect(ConnectorOut & out, ConnectorIn & in, float weight = 1.0f);
	void disconnect(ConnectorOut & out, ConnectorIn & in);

	void note_press  (int note, float velocity, int time_offset = 0);
	void note_release(int note, int time_offset = 0);

	void control_update(int control, float value);
	
	void open_file(char const * filename);
	void save_file(char const * filename) const;

private:
	struct Connection {
		ConnectorOut * out;
		ConnectorIn  * in;
		float        * weight;
	};

	std::vector<Connection> connections;
	std::optional<Connection> selected_connection;

	Connector * dragging = nullptr;
	
	Component * component_to_be_removed = nullptr;

	bool compute_update_order();

	void render_menu();
	void render_components();
	void render_connections();

	void render_connector_in (ConnectorIn  & in);
	void render_connector_out(ConnectorOut & out);
};
