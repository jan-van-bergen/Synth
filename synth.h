#pragma once
#include "components.h"

struct Synth {
	std::vector<Component *> sources;
	std::vector<Component *> sinks;

	std::vector<std::unique_ptr<Component>> components;
	
	template<typename T>
	T * add_component() {
		auto component = std::make_unique<T>();

		if (component->inputs .size() == 0) sources.push_back(component.get());
		if (component->outputs.size() == 0) sinks  .push_back(component.get());

		return static_cast<T *>(components.emplace_back(std::move(component)).get());
	}

	int tempo = 130;

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
	
	void connect(ConnectorOut & out, ConnectorIn & in) {
		out.others.push_back(&in);
		in .others.push_back(std::make_pair(&out, 1.0f));
	}

	void disconnect(ConnectorOut & out, ConnectorIn & in) {
		out.others.erase(std::find   (out.others.begin(), out.others.end(), &in));
		in .others.erase(std::find_if(in .others.begin(), in .others.end(), [&out](auto pair) {
			return pair.first == &out;	
		}));
	}

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
