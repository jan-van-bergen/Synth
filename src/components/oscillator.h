#pragma once
#include "voice.h"

struct OscillatorVoice : Voice {
	float phase  = 0.0f;
	float sample = 0.0f;

	OscillatorVoice(int note, float velocity, int start_time) : Voice(note, velocity, start_time) { }
};

struct OscillatorComponent : VoiceComponent<OscillatorVoice> {
	static constexpr const char * waveform_names[] = { "Sine", "Triangle", "Saw", "Square", "Pulse 25%", "Pulse 12.5%", "Noise" };

	int waveform_index = 2;
	
	Parameter<int>   invert = { this, "invert", "Inv", "Invert Waveform", 0, std::make_pair(0, 1), { } };
	Parameter<float> phase  = { this, "phase",  "Phs", "Phase Offset",    0, std::make_pair(0.0f, 1.0f), { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f } };

	Parameter<int>   transpose = { this, "transpose", "Tps", "Transpose (notes)", 0, std::make_pair(-24, 24), { -24, -12, 0, 12, 24 } };
	Parameter<float> detune    = { this, "detune",    "Dtn", "Detune (cents)", 0.0f, std::make_pair(-100.0f, 100.0f), { 0.0f } };

	Parameter<float> portamento = { this, "portamento", "Por",  "Portamento", 0.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };

	// Envelope
	Parameter<float> attack  = Parameter<float>::make_attack (this);
	Parameter<float> hold	 = Parameter<float>::make_hold   (this);
	Parameter<float> decay 	 = Parameter<float>::make_decay  (this);
	Parameter<float> sustain = Parameter<float>::make_sustain(this);
	Parameter<float> release = Parameter<float>::make_release(this);
	
	OscillatorComponent(int id) : VoiceComponent(id, "Oscillator", { { this, "MIDI In", true } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;

private:
	unsigned seed = util::seed();

	float portamento_frequency = 0.0f;
};
