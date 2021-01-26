#pragma once
#include <string>
#include <vector>
#include <optional>

#include <memory>
#include <unordered_map>

#include <SDL2/SDL.h>

#include "sample.h"

struct Note {
	int   note;
	float velocity;
	int   time;
};

struct Component;

struct Connector {
	const bool is_input;

	Component * component;

	std::string name;

	float pos[2];

	std::vector<Connector *> connected;

	Connector(bool is_input, Component * component, std::string const & name) : is_input(is_input), component(component), name(name) { }
};

struct ConnectorIn : Connector {
	std::vector<std::pair<struct ConnectorOut *, float>> others; 
	
	ConnectorIn(Component * component, std::string const & name) : Connector(true, component, name) { }

	Sample get_value(int i) const;
};

struct ConnectorOut : Connector {
	std::vector<struct ConnectorIn *> others; 
	
	Sample values[BLOCK_SIZE];

	ConnectorOut(Component * component, std::string const & name) : Connector(false, component, name) {
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

struct SequencerComponent : Component {
	static constexpr auto TRACK_SIZE = 16;
	float steps[TRACK_SIZE] = { };

	SequencerComponent() : Component("Sequencer", { }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct OscilatorComponent : Component {
	static constexpr const char * waveform_names[] = { "Sine", "Square", "Triangle", "Sawtooth" };

	int waveform_index = 3;
	
	int transpose = 0;

	// Envelope
	float env_attack  = 0.1f;
	float env_hold    = 0.5f;
	float env_decay   = 1.0f;
	float env_sustain = 0.5f;

	OscilatorComponent() : Component("Oscilator", { }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SamplerComponent : Component {
	std::vector<Sample> samples;
	int current_sample = 0;

	float velocity = 1.0f;

	char filename[128];

	SamplerComponent() : Component("Sampler", { { this, "Trigger" } }, { { this, "Out" } }) {
		strcpy_s(filename, "samples/kick.wav");
		load();
	}
	
	void load();

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct FilterComponent : Component {
	static constexpr char const * filter_names[] = { "Low Pass", "High Pass", "Band Pass", "None" };
	int filter_type = 0;

	float cutoff = 1000.0f;
	float resonance = 0.5f;

	FilterComponent() : Component("Filter", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	Sample state_1;
	Sample state_2;
};

struct DistortionComponent : Component {
	float amount = 0.5f;

	DistortionComponent() : Component("Distortion", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct DelayComponent : Component {
	int   steps    = 3;
	float feedback = 0.7f;

	std::vector<Sample> history;
	int offset = 0;

	DelayComponent() : Component("Delay", { { this, "In" } }, { { this, "Out" } }) {
		update_history_size();
	}

	void update_history_size() {
		history.resize(115 * SAMPLE_RATE * steps / 1000);
		offset %= history.size();
	}
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
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

	void render_connector_in (ConnectorIn  & in);
	void render_connector_out(ConnectorOut & out);
};
