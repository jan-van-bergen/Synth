#pragma once
#include <string>
#include <vector>
#include <optional>

#include <memory>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <ImGui/imgui.h>

#include "sample.h"

#include "parameter.h"
#include "connector.h"

struct Component {
	enum struct Type {
		SOURCE, // Entry point in Node Graph
		SINK,   // Exit point in Node Graph
		INTER   // Neither
	} type;

	std::string name;

	std::vector<ConnectorIn>  inputs;
	std::vector<ConnectorOut> outputs;

	Component(Type type, std::string const & name, std::vector<ConnectorIn> const & inputs, std::vector<ConnectorOut> const & outputs) : type(type), name(name), inputs(inputs), outputs(outputs) { }

	virtual void update(struct Synth const & synth) = 0;
	virtual void render(struct Synth const & synth) = 0;
};

struct SequencerComponent : Component {
	static constexpr auto TRACK_SIZE = 16;
	float steps[TRACK_SIZE] = { };

	int current_step = 0;

	SequencerComponent() : Component(Type::SOURCE, "Sequencer", { }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct OscilatorComponent : Component {
	static constexpr const char * waveform_names[] = { "Sine", "Square", "Triangle", "Sawtooth", "Noise" };

	int waveform_index = 3;
	
	Parameter<int>   transpose;
	Parameter<float> detune;

	// Envelope
	Parameter<float> attack;
	Parameter<float> hold;
	Parameter<float> decay;
	Parameter<float> sustain;

	OscilatorComponent() : Component(Type::SOURCE, "Oscilator", { }, { { this, "Out" } }),
		transpose("Transpose", 0, std::make_pair(-24, 24), { -24, -12, 0, 12, 24 }),	
		detune("Detune", 0.0f, std::make_pair(-100.0f, 100.0f), { 0.0f }),

		attack ("Attack",  0.1f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 }),
		hold   ("Hold",    0.5f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 }),
		decay  ("Decay",   1.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 }),
		sustain("Sustain", 0.5f, std::make_pair(0.0f, 1.0f))	
	{ }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SamplerComponent : Component {
	std::vector<Sample> samples;
	int current_sample = 0;

	float velocity = 0.0f;

	char filename[128];

	SamplerComponent() : Component(Type::INTER, "Sampler", { { this, "Trigger" } }, { { this, "Out" } }) {
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

	Parameter<float> cutoff;
	Parameter<float> resonance;

	FilterComponent() : Component(Type::INTER, "Filter", { { this, "In" } }, { { this, "Out" } }),
		cutoff   ("Cutoff",    1000.0f, std::make_pair(20.0f, 10000.0f)),
		resonance("Resonance",    0.5f, std::make_pair(0.0f, 1.0f))
	{ }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	Sample state_1;
	Sample state_2;
};

struct DistortionComponent : Component {
	Parameter<float> amount;

	DistortionComponent() : Component(Type::INTER, "Distortion", { { this, "In" } }, { { this, "Out" } }),
		amount("Amount", 0.5f, std::make_pair(0.0f, 1.0f))
	{ }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct DelayComponent : Component {
	Parameter<int>   steps;
	Parameter<float> feedback;

	std::vector<Sample> history;
	int offset = 0;

	DelayComponent() : Component(Type::INTER, "Delay", { { this, "In" } }, { { this, "Out" } }),
		steps("Steps", 3, std::make_pair(0, 8)),
		feedback("Feedback", 0.7f, std::make_pair(0.0f, 1.0f))
	{ }

	void history_resize(int size);
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SpeakerComponent : Component {
	SpeakerComponent() : Component(Type::SINK, "Speaker", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override { }
	void render(struct Synth const & synth) override { }
};

struct RecorderComponent : Component {
	std::vector<Sample> samples;

	bool recording = false;

	RecorderComponent() : Component(Type::INTER, "Recorder", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
