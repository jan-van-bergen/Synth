#pragma once
#include "voice.h"

#include "dsp/filter.h"

struct OscillatorVoice : Voice {
	float phase  = 0.0f;
	float sample = 0.0f;

	dsp::VAFilter<Sample> filter;

	OscillatorVoice(int note, float velocity, int start_time) : Voice(note, velocity, start_time) { }
};

struct OscillatorComponent : VoiceComponent<OscillatorVoice> {
	Parameter<int> waveform = { this, "waveform", "Wav", "Waveform", 2, std::make_pair(0, 6) };
	
	Parameter<int>   invert  = { this, "invert", "Inv", "Invert Waveform",     0, std::make_pair(0, 1) };
	Parameter<float> phase   = { this, "phase",  "Phs", "Phase Offset",        0, std::make_pair(0.0f, 1.0f), { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f } };
	Parameter<float> stereo  = { this, "stereo", "Ste", "Phase Stereo Offset", 0, std::make_pair(0.0f, 1.0f), { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f } };

	Parameter<int>   transpose = { this, "transpose", "Tps", "Transpose (notes)", 0, std::make_pair(-24, 24),         { -24, -12, 0, 12, 24 } };
	Parameter<float> detune    = { this, "detune",    "Dtn", "Detune (cents)", 0.0f, std::make_pair(-100.0f, 100.0f), { 0.0f } };

	Parameter<float> portamento = { this, "portamento", "Por",  "Portamento", 0.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 } };

	// Volume envelope
	Parameter<float> attack  = Parameter<float>::make_attack (this);
	Parameter<float> hold    = Parameter<float>::make_hold   (this);
	Parameter<float> decay   = Parameter<float>::make_decay  (this);
	Parameter<float> sustain = Parameter<float>::make_sustain(this);
	Parameter<float> release = Parameter<float>::make_release(this);
	
	// Filter envelope
	Parameter<float> flt_attack  = Parameter<float>::make_attack (this, "flt_attack");
	Parameter<float> flt_hold    = Parameter<float>::make_hold   (this, "flt_hold");
	Parameter<float> flt_decay   = Parameter<float>::make_decay  (this, "flt_decay");
	Parameter<float> flt_sustain = Parameter<float>::make_sustain(this, "flt_release", 1.0f);

	Parameter<float> flt_min       = { this, "flt_min",       "Min", "Minimum Filter Amount", 1.0f, std::make_pair( 0.0f, 1.0f) };
	Parameter<float> flt_multiply  = { this, "flt_multiply",  "Amt", "Filter Amount",         0.0f, std::make_pair(-1.0f, 1.0f) };
	Parameter<float> flt_resonance = { this, "flt_resonance", "Res", "Filter Resonance",      0.0f, std::make_pair( 0.0f, 1.0f) };
	
	OscillatorComponent(int id) : VoiceComponent(id, "Oscillator", { { this, "MIDI In", true } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	unsigned seed = util::seed();

	float portamento_frequency = 0.0f;
};
