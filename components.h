#pragma once
#include <string>
#include <vector>
#include <optional>

#include <memory>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <ImGui/imgui.h>

#include "sample.h"
#include "connector.h"

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

	void update_history_size();
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SpeakerComponent : Component{
	SpeakerComponent() : Component("Speaker", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override { }
	void render(struct Synth const & synth) override { }
};
