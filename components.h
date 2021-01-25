#pragma once
#include <string>
#include <vector>
#include <optional>

#include <memory>
#include <unordered_map>

#include "sample.h"

struct Note {
	int   note;
	float velocity;
	int   time;
};

struct Component;

struct Connector {
	Component * component;

	float pos[2];

	std::string name;

	Connector(Component * component, std::string const & name) : component(component), name(name) { }
};

struct ConnectorIn : Connector {
	struct ConnectorOut * other = nullptr; 
	
	ConnectorIn(Component * component, std::string const & name) : Connector(component, name) { }

	Sample get_value(int i) const;
};

struct ConnectorOut : Connector {
	struct ConnectorIn * other = nullptr; 
	
	Sample values[BLOCK_SIZE];

	ConnectorOut(Component * component, std::string const & name) : Connector(component, name) {
		clear();
	}

	void clear() { memset(values, 0, sizeof(values)); }
};

struct Component {
	std::string name;

	std::vector<ConnectorIn>  inputs;
	std::vector<ConnectorOut> outputs;

	Component(std::string const & name, std::vector<ConnectorIn> const & inputs, std::vector<ConnectorOut> const & outputs) : name(name), inputs(inputs), outputs(outputs) { }

	virtual void update(struct Synth const & synth) = 0;
	virtual void render(struct Synth const & synth) = 0;
};

struct OscilatorComponent : Component {
//	enum struct Waveform { SINE, SQUARE, TRIANGLE, SAWTOOTH } waveform = Waveform::SINE;

	static constexpr const char * options[] = { "Sine", "Square", "Triangle", "Sawtooth" };

	int waveform_index = 3;
	

	OscilatorComponent() : Component("Oscilator", { }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct FilterComponent : Component {
	float cutoff = 1000.0f;
	float resonance = 0.5f;

	FilterComponent() : Component("Filter", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	Sample state_1;
	Sample state_2;
};

struct DelayComponent : Component {
	float feedback = 0.7f;

	DelayComponent() : Component("Delay", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	static constexpr int HISTORY_SIZE = SAMPLE_RATE * 462 / 1000;
	Sample history[HISTORY_SIZE];

	int offset = 0;
};

struct SpeakerComponent : Component{
	SpeakerComponent() : Component("Speaker", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};


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

	int time = 0;
	
	std::unordered_map<int, Note>  notes;
	std::unordered_map<int, float> controls = { { 0x4A, 0.5f } };
	
	void update(Sample buf[BLOCK_SIZE]);
	void render();
	
	void connect(ConnectorOut & out, ConnectorIn & in) {
		out.other = &in;
		in .other = &out;
	}

	void disconnect(ConnectorOut & out, ConnectorIn & in) {
		out.other = nullptr;
		in .other = nullptr;
	}

	void note_press(int note, float velocity = 1.0f) {
		Note n = { note, velocity, time };	
		notes.insert(std::make_pair(note, n));
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

	void render_oscilators();
	void render_speakers();

	void render_connector_in (ConnectorIn  & in);
	void render_connector_out(ConnectorOut & out);
};
