#pragma once
#include "components.h"

struct Synth {
	std::vector<Component *> sources;
	std::vector<Component *> sinks;

	std::vector<std::unique_ptr<Component>> components;
	
	template<typename T>
	T * add_component() {
		auto component = std::make_unique<T>();

		if (component->type == Component::Type::SOURCE) sources.push_back(component.get());
		if (component->type == Component::Type::SINK)   sinks  .push_back(component.get());
		
		return static_cast<T *>(components.emplace_back(std::move(component)).get());
	}

	Parameter<int> tempo = { "Tempo", 130, std::make_pair(60, 200), { 80, 110, 128, 140, 150, 174 } };

	int time = 0;
	
	struct Note {
		int   note;
		float velocity;
		int   time;
	};

	std::unordered_map<int, Note>  notes;
	std::unordered_map<int, float> controls = { { 0x4A, 0.5f } };
	
	void update(Sample buf[BLOCK_SIZE]);
	void render();
	
	void    connect(ConnectorOut & out, ConnectorIn & in);
	void disconnect(ConnectorOut & out, ConnectorIn & in);

	void note_press(int note, float velocity = 1.0f) {
		notes.insert(std::make_pair(note, Note { note, velocity, time }));
	}
	void note_release(int note) {
		notes.erase(note);
	}

	void control_update(int control, float value) {
		controls[control] = value;
	}

private:
	using Connection = std::pair<ConnectorOut *, ConnectorIn *>;

	std::vector<Connection> connections;
	std::optional<Connection> selected_connection;

	Connector * dragging = nullptr;
	bool        drag_handled;

	void render_connector_in (ConnectorIn  & in);
	void render_connector_out(ConnectorOut & out);
};
