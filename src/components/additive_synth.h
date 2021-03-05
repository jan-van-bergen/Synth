#pragma once
#include "voice.h"

struct AdditiveVoice : Voice {
	float sample = 0.0f;

	AdditiveVoice(int note, float velocity, int start_time) : Voice(note, velocity, start_time) { }
};

struct AdditiveSynthComponent : VoiceComponent<AdditiveVoice> {
	static constexpr auto NUM_HARMONICS = 32;

	float harmonics[NUM_HARMONICS] = { 1.0f };
	
	// Envelope
	Parameter<float> attack  = Parameter<float>::make_attack (this);
	Parameter<float> hold	 = Parameter<float>::make_hold   (this);
	Parameter<float> decay 	 = Parameter<float>::make_decay  (this);
	Parameter<float> sustain = Parameter<float>::make_sustain(this);
	Parameter<float> release = Parameter<float>::make_release(this);
	
	AdditiveSynthComponent(int id) : VoiceComponent(id, "Additive Synth", { { this, "MIDI In", true } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;
};
