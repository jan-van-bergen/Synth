#pragma once
#include <string>
#include <vector>

#include <memory>

#include <SDL2/SDL.h>
#include <ImGui/imgui.h>

#include "sample.h"
#include "midi.h"

#include "parameter.h"
#include "connector.h"

#include "meta.h"

#include "json.h"

struct Component {
	std::string name;

	std::vector<ConnectorIn>  inputs;
	std::vector<ConnectorOut> outputs;

	int const id;
	float pos [2] = { };
	float size[2] = { };

	Component(int id,
		std::string && name,
		std::vector<ConnectorIn>  && inputs,
		std::vector<ConnectorOut> && outputs) : id(id), name(name), inputs(inputs), outputs(outputs) { }

	virtual void update(struct Synth const & synth) = 0;
	virtual void render(struct Synth const & synth) = 0;

	virtual void   serialize(json::Writer & writer) const { }
	virtual void deserialize(json::Object const & object) { }
};

struct KeyboardComponent : Component {
	KeyboardComponent(int id) : Component(id, "Keyboard", { }, { { this, "MIDI Out", true } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SequencerComponent : Component {
	static constexpr auto TRACK_SIZE = 16;
	float pattern[TRACK_SIZE] = { };

	int current_step = 0;

	SequencerComponent(int id) : Component(id, "Sequencer", { }, { { this, "MIDI Out", true } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;
};

struct PianoRollComponent : Component {
	static constexpr char const * DEFAULT_FILENAME = "midi/melody_2.mid";

	midi::Track midi;
	int         midi_offset;
	int         midi_length;

	char filename[128];

	PianoRollComponent(int id) : Component(id, "Piano Roll", { }, { { this, "MIDI Out", true } }) {
		strcpy_s(filename, DEFAULT_FILENAME);
		reload_file();
	}

	void reload_file() {
		auto track = midi::Track::load(filename);

		if (!track.has_value()) {
			midi_offset = 0;
			midi_length = 0;
			return;
		}

		midi = std::move(track.value());

		if (midi.events.size() == 0) {
			midi_length = 0;
		} else {		
			midi_length = util::round_up(midi.events[midi.events.size() - 1].time, 4 * midi.ticks);
		}

		midi_offset = 0;
	}

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
	
	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;
};

struct OscillatorComponent : Component {
	static constexpr const char * waveform_names[] = { "Sine", "Triangle", "Saw", "Square", "Pulse 25%", "Pulse 12.5%", "Noise" };

	int waveform_index = 2;
	
	Parameter<int>   invert = { "Invert", 0, std::make_pair(0, 1), { } };
	Parameter<float> phase  = { "Phase",  0, std::make_pair(0.0f, 1.0f), { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f } };

	struct Voice {
		int   note;
		float velocity;
		
		int start_time; // In samples

		float phase  = 0.0f;
		float sample = 0.0f;

		float release_time = INFINITY; // In steps

		Voice(int note, float velocity, int start_time) : note(note), velocity(velocity), start_time(start_time) { } 
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

	OscillatorComponent(int id) : Component(id, "Oscillator", { { this, "MIDI In", true } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;

private:
	float portamento_frequency = 0.0f;
};

struct SamplerComponent : Component {
	static constexpr char const * DEFAULT_FILENAME = "samples/kick.wav";

	std::vector<Sample> samples;
	
	char filename[128];

	Parameter<int> base_note = { "Base Note", 36, std::make_pair(0, 127), { 0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120 } };

	SamplerComponent(int id) : Component(id, "Sampler", { { this, "MIDI In", true } }, { { this, "Out" } }) {
		strcpy_s(filename, DEFAULT_FILENAME);
		samples = util::load_wav(filename);
	}
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;

private:
	struct Voice {
		float current_sample = 0.0f;
		float step           = 0.0f;

		float velocity = 1.0f;
	};
	std::vector<Voice> voices;
};

struct SplitComponent : Component {
	Parameter<float> mix = { "Mix A/B", 0.5f, std::make_pair(0.0f, 1.0f) };

	SplitComponent(int id) : Component(id, "Split", { { this, "In" } }, { { this, "Out A" }, { this, "Out B" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;
};

struct PanComponent : Component {
	Parameter<float> pan = { "Pan", 0.0f, std::make_pair(-1.0f, 1.0f) };

	PanComponent(int id) : Component(id, "Pan", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;
};

struct FilterComponent : Component {
	static constexpr char const * filter_names[] = { "Low Pass", "High Pass", "Band Pass", "None" };
	int filter_type = 0;

	Parameter<float> cutoff    = { "Cutoff",    1000.0f, std::make_pair(20.0f, 20000.0f), { 50, 100, 200, 400, 800, 1600, 3200, 6400, 12800, 19200 }, Param::Curve::LOGARITHMIC };
	Parameter<float> resonance = { "Resonance",    0.5f, std::make_pair(0.0f, 1.0f) };

	FilterComponent(int id) : Component(id, "Filter", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;

private:
	Sample state_1;
	Sample state_2;
};

struct DelayComponent : Component {
	Parameter<int>   steps    = { "Steps", 3, std::make_pair(1, 8) };
	Parameter<float> feedback = { "Feedback", 0.7f, std::make_pair(0.0f, 1.0f) };

	std::vector<Sample> history;
	int offset = 0;

	DelayComponent(int id) : Component(id, "Delay", { { this, "In" } }, { { this, "Out" } }) { }

	void history_resize(int size);
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;
};

struct FlangerComponent : Component {
	Parameter<float> delay    = { "Delay",    0.5f,  std::make_pair(0.001f, 10.0f) };
	Parameter<float> depth    = { "Depth",    0.5f,  std::make_pair(0.001f, 10.0f) };
	Parameter<float> rate     = { "Rate",     1.0f,  std::make_pair(0.0f,    5.0f) };
	Parameter<float> phase    = { "Phase",    0.02f, std::make_pair(0.0f,    1.0f) };
	Parameter<float> feedback = { "Feedback", 0.2f,  std::make_pair(0.0f,    1.0f) };
	Parameter<float> drywet   = { "Dry/Wet",  0.7f,  std::make_pair(0.0f,    1.0f) };

	FlangerComponent(int id) : Component(id, "Flanger", { { this, "In" } }, { { this, "out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
	
	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;

private:
	static constexpr auto MAX_DELAY_IN_SECONDS = 1;
	static constexpr auto HISTORY_SIZE = MAX_DELAY_IN_SECONDS * SAMPLE_RATE;

	float history_left [HISTORY_SIZE] = { };
	float history_right[HISTORY_SIZE] = { };
	int   history_offset = 0;

	float lfo_phase = 0.0f;
};

struct DistortionComponent : Component {
	Parameter<float> amount = { "Amount", 0.5f, std::make_pair(0.0f, 1.0f) };

	DistortionComponent(int id) : Component(id, "Distortion", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;
};

struct BitCrusherComponent : Component {
	Parameter<float> gain = { "Gain",             1.0f, std::make_pair(1.0f, 8.0f) };
	Parameter<int>   bits = { "Bits",             32,   std::make_pair(1, 32) };
	Parameter<int>   rate = { "Sample Reduction",  1,   std::make_pair(1, 128) };

	BitCrusherComponent(int id) : Component(id, "Bit Crusher", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;
};

struct CompressorComponent : Component {
	Parameter<float> threshold = { "Threshold", 0.0f, std::make_pair(-60.0f,   0.0f) };
	Parameter<float> ratio     = { "Ratio",     1.0f, std::make_pair(  1.0f,  30.0f) };
	Parameter<float> gain      = { "Gain",      0.0f, std::make_pair(-30.0f,  30.0f) };
	Parameter<float> attack    = { "Attack",   15.0f, std::make_pair(  0.0f, 400.0f) };
	Parameter<float> release   = { "Release", 200.0f, std::make_pair(  0.0f, 400.0f) };

	CompressorComponent(int id) : Component(id, "Compressor", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize(json::Writer & writer) const override;
	void deserialize(json::Object const & object) override;

private:
	float env = 0.0f;
};

struct SpeakerComponent : Component {
	std::vector<Sample> recorded_samples;
	bool                recording = false;

	SpeakerComponent(int id) : Component(id, "Speaker", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SpectrumComponent : Component {
	static constexpr auto      N = 4 * 1024;
	static constexpr auto log2_N = util::log2(N);

	static_assert(util::is_power_of_two(N)); // Required for FFT

	float spectrum[N] = { };

	SpectrumComponent(int id) : Component(id, "Spectrum", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct OscilloscopeComponent : Component {
	float samples[BLOCK_SIZE] = { };

	OscilloscopeComponent(int id) : Component(id, "Oscilloscope", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct DecibelComponent : Component {
	float decibels = -INFINITY;

	DecibelComponent(int id) : Component(id, "Decibel Meter", { { this, "Input" } }, { }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};



template<typename T>
concept IsComponent = std::derived_from<T, Component>;

template<IsComponent ... Ts>
using ComponentTypeList = meta::TypeList<Ts ...>;

using AllComponents = ComponentTypeList<
	BitCrusherComponent,
	CompressorComponent,
	DecibelComponent,
	DelayComponent,
	DistortionComponent,
	FilterComponent,
	FlangerComponent,
	KeyboardComponent,
	OscillatorComponent,
	OscilloscopeComponent,
	PanComponent,
	PianoRollComponent,
	SamplerComponent,
	SequencerComponent,
	SpeakerComponent,
	SpectrumComponent,
	SplitComponent
>; // TypeList of all Components, used for deserialization. Synth::add_component<T> will only accept T that occur in this TypeList.
