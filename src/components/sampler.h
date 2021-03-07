#pragma once
#include "voice.h"

struct SamplerVoice : Voice {
	float sample = 0.0f;
	
	SamplerVoice(int note, float velocity, float start_time) : Voice(note, velocity, start_time) { }
};

struct SamplerComponent : VoiceComponent<SamplerVoice> {
	static constexpr auto DEFAULT_FILENAME = "samples/kick.wav";

	static constexpr auto VISUAL_NUM_SAMPLES = 512;

	std::vector<Sample> samples;
	
	std::string  filename;
	char const * filename_display;

	Parameter<int> base_note = { this, "base_note", "Note", "Base Note", util::note<util::NoteName::C, 3>(), std::make_pair(0, 127), { 0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120 } };
	
	// Envelope
	Parameter<float> attack  = Parameter<float>::make_attack (this, "attack", 0.0f);
	Parameter<float> hold	 = Parameter<float>::make_hold   (this);
	Parameter<float> decay 	 = Parameter<float>::make_decay  (this);
	Parameter<float> sustain = Parameter<float>::make_sustain(this, "sustain", 1.0f);
	Parameter<float> release = Parameter<float>::make_release(this);
	
	SamplerComponent(int id) : VoiceComponent(id, "Sampler", { { this, "MIDI In", true } }, { { this, "Out" } }) {
		load(DEFAULT_FILENAME);
	}

	void load(char const * filename);
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;

private:
	struct {
		std::vector<float> samples;
		float max_y;
	} visual;
};
