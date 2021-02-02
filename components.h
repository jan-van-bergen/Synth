#pragma once
#include <string>
#include <vector>
#include <optional>

#include <memory>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <ImGui/imgui.h>

#include "sample.h"
#include "midi.h"

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

	Component(Type type, std::string && name, std::vector<ConnectorIn> && inputs, std::vector<ConnectorOut> && outputs) : type(type), name(name), inputs(inputs), outputs(outputs) { }

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

struct PianoRollComponent : Component {
	midi::Track midi = midi::Track::load("melody_2.mid");
	int         midi_offset = 0;
	int         midi_rounds = 0;
	
	int midi_length;

	PianoRollComponent() : Component(Type::SOURCE, "Piano Roll", { }, { { this, "Out" } }) {
		if (midi.events.size() == 0) {
			midi_length = 0;
		} else {		
			midi_length = util::round_up(midi.events[midi.events.size() - 1].time, 4 * midi.ticks);
		}
	}

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct OscillatorComponent : Component {
	static constexpr const char * waveform_names[] = { "Sine", "Square", "Triangle", "Sawtooth", "Noise" };

	int waveform_index = 3;
	
	struct Voice {
		int   note;
		float velocity;

		float sample = 0.0f;
		float phase  = 0.0f;

		float release_time = INFINITY;
	};

	std::vector<Voice> voices;

	Parameter<int>   transpose = { "Transpose", 0, std::make_pair(-24, 24), { -24, -12, 0, 12, 24 } };
	Parameter<float> detune    = { "Detune", 0.0f, std::make_pair(-100.0f, 100.0f), { 0.0f } };

	Parameter<float> portamento = { "Portamento",  0.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };

	// Envelope
	Parameter<float> attack  = { "Attack",  0.1f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };
	Parameter<float> hold	 = { "Hold",    0.5f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };
	Parameter<float> decay 	 = { "Decay",   1.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };
	Parameter<float> sustain = { "Sustain", 0.5f, std::make_pair(0.0f, 1.0f) };
	Parameter<float> release = { "Release", 0.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };

	OscillatorComponent() : Component(Type::SOURCE, "Oscillator", { }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	float portamento_frequency = 0.0f;
};

struct WaveTableComponent : Component {
	std::vector<Sample> samples = util::load_wav("samples/piano.wav");
	float current_sample = 0.0f;

	WaveTableComponent() : Component(Type::SOURCE, "Wave Table", { }, { { this, "Out" } }) { }
	
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
		samples = util::load_wav(filename);
	}
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SplitComponent : Component {
	Parameter<float> mix = { "Mix A/B", 0.5f, std::make_pair(0.0f, 1.0f) };

	SplitComponent() : Component(Type::INTER, "Split", { { this, "In" } }, { { this, "Out A" }, { this, "Out B" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct PanComponent : Component {
	Parameter<float> pan = { "Pan", 0.0f, std::make_pair(-1.0f, 1.0f) };

	PanComponent() : Component(Type::INTER, "Pan", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct FilterComponent : Component {
	static constexpr char const * filter_names[] = { "Low Pass", "High Pass", "Band Pass", "None" };
	int filter_type = 0;

	Parameter<float> cutoff    = { "Cutoff",    1000.0f, std::make_pair(20.0f, 20000.0f), { 50, 100, 200, 400, 800, 1600, 3200, 6400, 12800, 19200 }, Param::Curve::LOGARITHMIC };
	Parameter<float> resonance = { "Resonance",    0.5f, std::make_pair(0.0f, 1.0f) };

	FilterComponent() : Component(Type::INTER, "Filter", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	Sample state_1;
	Sample state_2;
};

struct DistortionComponent : Component {
	Parameter<float> amount = { "Amount", 0.5f, std::make_pair(0.0f, 1.0f) };

	DistortionComponent() : Component(Type::INTER, "Distortion", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct DelayComponent : Component {
	Parameter<int>   steps    = { "Steps", 3, std::make_pair(1, 8) };
	Parameter<float> feedback = { "Feedback", 0.7f, std::make_pair(0.0f, 1.0f) };

	std::vector<Sample> history;
	int offset = 0;

	DelayComponent() : Component(Type::INTER, "Delay", { { this, "In" } }, { { this, "Out" } }) { }

	void history_resize(int size);
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SpeakerComponent : Component {
	std::vector<Sample> recorded_samples;
	bool                recording = false;

	SpeakerComponent() : Component(Type::SINK, "Speaker", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SpectrumComponent : Component {
	static constexpr auto      N = 4 * 1024;
	static constexpr auto log2_N = util::log2(N);

	static_assert(util::is_power_of_two(N)); // Required for FFT

	float spectrum[N] = { };

	SpectrumComponent() : Component(Type::INTER, "Spectrum", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
