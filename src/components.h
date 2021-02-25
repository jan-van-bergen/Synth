#pragma once
#include <string>
#include <vector>

#include <memory>

#include <SDL2/SDL.h>
#include <ImGui/imgui.h>

#include "sample.h"
#include "midi.h"

#include "dsp/filter.h"

#include "parameter.h"
#include "connector.h"

#include "meta.h"

struct Component {
	std::string name;

	std::vector<ConnectorIn>  inputs;
	std::vector<ConnectorOut> outputs;

	int const id;
	float pos [2] = { };
	float size[2] = { };

	Component(int id,
		std::string name,
		std::vector<ConnectorIn>  && inputs,
		std::vector<ConnectorOut> && outputs) : id(id), name(name), inputs(inputs), outputs(outputs) { }

	virtual void update(struct Synth const & synth) = 0;
	virtual void render(struct Synth const & synth) = 0;

	void serialize(json::Writer & writer) const {
		writer.write("id",     id);
		writer.write("pos_x",  pos[0]);
		writer.write("pos_y",  pos[1]);
		writer.write("size_x", size[0]);
		writer.write("size_y", size[1]);

		for (auto param : params) {
			param->serialize(writer);
		}

		serialize_custom(writer);
	}
	
	void deserialize(json::Object const & object) {
		pos [0] = object.find_float("pos_x", 100.0f);
		pos [1] = object.find_float("pos_y", 100.0f);
		size[0] = object.find_float("size_x");
		size[1] = object.find_float("size_y");

		for (auto param : params) {
			param->deserialize(object);
		}

		deserialize_custom(object);
	}

protected:
	friend Param;
	std::vector<Param *> params;
	
	virtual void   serialize_custom(json::Writer & writer) const { }
	virtual void deserialize_custom(json::Object const & object) { }
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

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;
};

struct MIDIPlayerComponent : Component {
	static constexpr char const * DEFAULT_FILENAME = "midi/melody_2.mid";

	midi::Track midi;
	int         midi_offset;
	int         midi_length;

	char filename[128];

	MIDIPlayerComponent(int id) : Component(id, "MIDI Player", { }, { { this, "MIDI Out", true } }) {
		strcpy_s(filename, DEFAULT_FILENAME);
		reload_file();
	}

	void reload_file();

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
	
	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;

private:
	std::vector<Note> notes;
};

struct ImproviserComponent : Component {
	Parameter<int> tonality = { this, "tonality", "Ton", "Tonality", 0, std::make_pair(0, 11) };

	static constexpr char const * mode_names[] = { "Major", "Minor" };
	enum struct Mode { MAJOR, MINOR } mode = Mode::MAJOR;

	Parameter<int> num_notes = { this, "num_notes", "Num", "Number of notes in chord", 3, std::make_pair(1, 5) };

	ImproviserComponent(int id) : Component(id, "Improviser", { }, { { this, "MIDI Out", true } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
	
	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;

private:
	std::vector<int> chord;
	int current_chord = 0;
	int current_time  = std::numeric_limits<int>::max();
};

struct ArpComponent : Component {
	static constexpr char const * mode_names[] = { "Up", "Down", "Up/Down", "Random" };

	enum struct Mode { UP, DOWN, UPDOWN, RANDOM } mode = Mode::UP;

	Parameter<float> steps = { this, "steps", "Stp", "Length in Steps", 1.0f, std::make_pair(0.25f, 16.0f), { 0.25f, 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 8.0f, 16.0f } };

	ArpComponent(int id) : Component(id, "Arp", { { this, "MIDI In", true } }, { { this, "MIDI Out", true } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	std::vector<Note> notes;

	int   current_note = 0;
	float current_time = 0.0f;

	bool going_up = true;
};

struct OscillatorComponent : Component {
	static constexpr const char * waveform_names[] = { "Sine", "Triangle", "Saw", "Square", "Pulse 25%", "Pulse 12.5%", "Noise" };

	int waveform_index = 2;
	
	Parameter<int>   invert = { this, "invert", "Inv", "Invert Waveform", 0, std::make_pair(0, 1), { } };
	Parameter<float> phase  = { this, "phase",  "Phs", "Phase Offset",    0, std::make_pair(0.0f, 1.0f), { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f } };

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

	Parameter<int>   transpose = { this, "transpose", "Tps", "Transpose (notes)", 0, std::make_pair(-24, 24), { -24, -12, 0, 12, 24 } };
	Parameter<float> detune    = { this, "detune",    "Dtn", "Detune (cents)", 0.0f, std::make_pair(-100.0f, 100.0f), { 0.0f } };

	Parameter<float> portamento = { this, "portamento", "Por",  "Portamento", 0.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };

	// Envelope
	Parameter<float> attack  = { this, "attack",  "Att",  "Attack",  0.1f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };
	Parameter<float> hold	 = { this, "hold",    "Hold", "Hold",    0.5f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };
	Parameter<float> decay 	 = { this, "decay",   "Dec",  "Decay",   1.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };
	Parameter<float> sustain = { this, "sustain", "Sus",  "Sustain", 0.5f, std::make_pair(0.0f, 1.0f) };
	Parameter<float> release = { this, "release", "Rel",  "Release", 0.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };

	OscillatorComponent(int id) : Component(id, "Oscillator", { { this, "MIDI In", true } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;

private:
	float portamento_frequency = 0.0f;
};

struct SamplerComponent : Component {
	static constexpr auto DEFAULT_FILENAME = "samples/kick.wav";

	static constexpr auto VISUAL_NUM_SAMPLES = 512;

	std::vector<Sample> samples;
	
	char filename[128];

	Parameter<int> base_note = { this, "base_note", "Note", "Base Note", 36, std::make_pair(0, 127), { 0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120 } };

	SamplerComponent(int id) : Component(id, "Sampler", { { this, "MIDI In", true } }, { { this, "Out" } }) {
		strcpy_s(filename, DEFAULT_FILENAME);
		load();
	}

	void load();
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;

private:
	struct Voice {
		float current_sample = 0.0f;
		float step           = 0.0f;

		float velocity = 1.0f;
	};
	std::vector<Voice> voices;

	struct {
		std::vector<float> samples;
		float max_y;
	} visual;
};

struct SplitComponent : Component {
	Parameter<float> mix = { this, "mix", "A/B", "Mix A/B", 0.5f, std::make_pair(0.0f, 1.0f), { 0.0f, 0.5f, 1.0f } };

	SplitComponent(int id) : Component(id, "Split", { { this, "In" } }, { { this, "Out A" }, { this, "Out B" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct PanComponent : Component {
	Parameter<float> pan = { this, "pan", "Pan", "Pan", 0.0f, std::make_pair(-1.0f, 1.0f), { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f } };

	PanComponent(int id) : Component(id, "Pan", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct FilterComponent : Component {
	static constexpr char const * filter_names[] = { "Low Pass", "High Pass", "Band Pass", "None" };
	int filter_type = 0;

	Parameter<float> cutoff    = { this, "cutoff",    "Freq", "Cutoff Frequency", 1000.0f, std::make_pair(20.0f, 20000.0f), { 50, 100, 200, 400, 800, 1600, 3200, 6400, 12800, 19200 }, Param::Curve::LOGARITHMIC };
	Parameter<float> resonance = { this, "resonance", "Res",  "Resonance",           0.5f, std::make_pair(0.0f, 1.0f) };

	FilterComponent(int id) : Component(id, "Filter", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;

private:
	dsp::VAFilter<Sample> filter;
};

struct DelayComponent : Component {
	Parameter<int>   steps    = { this, "steps",    "Del", "Delay in Steps",  3,    std::make_pair(1, 8) };
	Parameter<float> feedback = { this, "feedback", "FB",  "Feedback Amount", 0.7f, std::make_pair(0.0f, 1.0f) };

	std::vector<Sample> history;
	int offset = 0;

	DelayComponent(int id) : Component(id, "Delay", { { this, "In" } }, { { this, "Out" } }) { }

	void history_resize(int size);
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct FlangerComponent : Component {
	Parameter<float> delay    = { this, "delay",    "Del",  "Delay",                               0.5f,  std::make_pair(0.001f, 10.0f) };
	Parameter<float> depth    = { this, "depth",    "Dep",  "Depth",                               0.5f,  std::make_pair(0.001f, 10.0f) };
	Parameter<float> rate     = { this, "rate",     "Rate", "LFO Frequency",                       1.0f,  std::make_pair(0.0f,    5.0f) };
	Parameter<float> phase    = { this, "phase",    "Phs",  "Phase Offset between Left and Right", 0.02f, std::make_pair(0.0f,    1.0f), { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f } };
	Parameter<float> feedback = { this, "feedback", "FB",   "Feedback Amount",                     0.2f,  std::make_pair(0.0f,    1.0f) };
	Parameter<float> drywet   = { this, "drywet",   "D/W",  "Dry/Wet Ratio",                       0.7f,  std::make_pair(0.0f,    1.0f) };

	FlangerComponent(int id) : Component(id, "Flanger", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	static constexpr auto MAX_DELAY_IN_SECONDS = 1;
	static constexpr auto HISTORY_SIZE = MAX_DELAY_IN_SECONDS * SAMPLE_RATE;

	float history_left [HISTORY_SIZE] = { };
	float history_right[HISTORY_SIZE] = { };
	int   history_offset = 0;

	float lfo_phase = 0.0f;
};

struct PhaserComponent : Component {
	Parameter<float> rate       = { this, "rate",       "Rate", "LFO Frequency",                       1.0f,  std::make_pair(0.0f, 5.0f) };
	Parameter<float> min_depth  = { this, "min_depth",  "Min",  "Minimum Depth",                     500.0f,  std::make_pair(20.0f, 20000.0f), { }, Param::Curve::LOGARITHMIC };
	Parameter<float> max_depth  = { this, "max_depth",  "Max",  "Maximum Depth",                    1000.0f,  std::make_pair(20.0f, 20000.0f), { }, Param::Curve::LOGARITHMIC };
	Parameter<float> phase      = { this, "phase",      "Phs",  "Phase Offset bewteen Left and Right", 0.02f, std::make_pair(0.0f, 1.0f) };
	Parameter<int>   num_stages = { this, "num_stages", "Num",  "Number of Stages",                    10,    std::make_pair(0, 32) };
	Parameter<float> feedback   = { this, "feedback",   "FB",   "Feedback Amount",                     0.2f,  std::make_pair(0.0f, 0.9999f) };
	Parameter<float> drywet     = { this, "drywet",     "D/W",  "Dry/Wet Ratio",                       0.7f,  std::make_pair(0.0f, 1.0f) };

	PhaserComponent(int id) : Component(id, "Phaser", { { this, "In" } }, { { this, "out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
	
private:
	dsp::BiQuadFilter<float> all_pass_left  = { };
	dsp::BiQuadFilter<float> all_pass_right = { };

	Sample feedback_sample = { };

	float lfo_phase = 0.0f;
};

struct DistortionComponent : Component {
	Parameter<float> amount = { this, "amount", "Amt", "Amount", 0.5f, std::make_pair(0.0f, 1.0f) };

	DistortionComponent(int id) : Component(id, "Distortion", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct BitCrusherComponent : Component {
	Parameter<float> gain = { this, "gain", "Gain", "Gain (dB)",             0.0f, std::make_pair(0.0f, 9.0f) };
	Parameter<int>   bits = { this, "bits", "Bits", "Bits",                  32,   std::make_pair(1, 32) };
	Parameter<int>   rate = { this, "rate", "Rate", "Sample Rate Reduction",  1,   std::make_pair(1, 128) };

	BitCrusherComponent(int id) : Component(id, "Bit Crusher", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct EqualizerComponent : Component {
	static constexpr auto NUM_BANDS = 4;

	struct Band {
		static constexpr auto MAX_Q = 10.0f;

		static constexpr char const * mode_names[] = {
			"Low Pass",
			"Band Pass",
			"High Pass",
			"Peak",
			"Notch",
			"Low Shelf",
			"High Shelf"
		};
		int mode = 3;

		float freq;
		float Q    = 1.0f;
		float gain = 0.0f;

		dsp::BiQuadFilter<Sample> filter;

		Band(float freq) : freq(freq) { }
	};

	Band bands[NUM_BANDS] = { 63.0f, 294.0f, 1363.0f, 6324.0f };

	EqualizerComponent(int id) : Component(id, "Equalizer", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;
};

struct CompressorComponent : Component {
	Parameter<float> threshold = { this, "threshold", "Thrs", "Threshold (dB)", 0.0f, std::make_pair(-60.0f,   0.0f) };
	Parameter<float> ratio     = { this, "ratio",     "Rat",  "Ratio",          1.0f, std::make_pair(  1.0f,  30.0f) };
	Parameter<float> gain      = { this, "gain",      "Gain", "Gain (dB)",      0.0f, std::make_pair(-30.0f,  30.0f) };
	Parameter<float> attack    = { this, "Attack",    "Att",  "Attack",        15.0f, std::make_pair(  0.0f, 400.0f) };
	Parameter<float> release   = { this, "Release",   "Rel",  "Release",      200.0f, std::make_pair(  0.0f, 400.0f) };

	CompressorComponent(int id) : Component(id, "Compressor", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	float env = 0.0f;
};

struct VocoderComponent : Component {
	struct Band {
		float freq = 1000.0f;

		dsp::BiQuadFilter<Sample> filter_mod;
		dsp::BiQuadFilter<Sample> filter_car;

		float gain = 0.0f;
	};
	
	std::vector<Band> bands;

	Parameter<int> num_bands = { this, "num_bands", "Bands", "Number of Bands", 16, std::make_pair(1, 64) };

	Parameter<float> width = { this, "width", "Q",         "Width", 5.0f, std::make_pair(0.0f, 10.0f) };
	Parameter<float> decay = { this, "decay", "Dec",       "Decay", 0.5f, std::make_pair(0.0f,  1.0f) };
	Parameter<float> gain  = { this, "gain",  "Gain (dB)", "Gain",  0.0f, std::make_pair(0.0f, 30.0f) };
	
	VocoderComponent(int id) : Component(id, "Vocoder", { { this, "Modulator" }, { this, "Carrier" } }, { { this, "Out" } }) {
		calc_bands();
	}

	void calc_bands();

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void deserialize_custom(json::Object const & object) override;
};

struct SpeakerComponent : Component {
	std::vector<Sample> recorded_samples;
	bool                recording = false;

	SpeakerComponent(int id) : Component(id, "Speaker", { { this, "Input" } }, { { this, "Pass" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

struct SpectrumComponent : Component {
	static constexpr auto N = 4 * 1024;

	float magnitudes[N / 2] = { };

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

private:
	static constexpr auto HISTORY_LENGTH_IN_SECONDS = 1.5f;
	static constexpr auto HISTORY_LENGTH            = int(HISTORY_LENGTH_IN_SECONDS * SAMPLE_RATE / BLOCK_SIZE);

	std::vector<float> history;
	int                history_index = 0;

	float previous_height_factor = 0.0f;
};

struct VectorscopeComponent : Component {
	static constexpr auto NUM_SAMPLES = 2 * 1024;

	Sample samples[NUM_SAMPLES] = { };
	int    sample_offset = 0;

	VectorscopeComponent(int id) : Component(id, "Vectorscope", { { this, "Input" } }, { }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};

template<typename T>
concept IsComponent = std::derived_from<T, Component>;

template<IsComponent ... Ts>
using ComponentTypeList = meta::TypeList<Ts ...>;

using AllComponents = ComponentTypeList<
	ArpComponent,
	BitCrusherComponent,
	CompressorComponent,
	DecibelComponent,
	DelayComponent,
	DistortionComponent,
	EqualizerComponent,
	FilterComponent,
	FlangerComponent,
	ImproviserComponent,
	KeyboardComponent,
	OscillatorComponent,
	OscilloscopeComponent,
	PanComponent,
	PhaserComponent,
	MIDIPlayerComponent,
	SamplerComponent,
	SequencerComponent,
	SpeakerComponent,
	SpectrumComponent,
	SplitComponent,
	VectorscopeComponent,
	VocoderComponent
>; // TypeList of all Components, used for deserialization. Synth::add_component<T> will only accept T that occur in this TypeList.
