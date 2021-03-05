#pragma once
#include "voice.h"

static constexpr auto FM_NUM_OPERATORS = 4;

struct FMVoice : Voice {
	float sample = 0.0f;

	float operator_values[FM_NUM_OPERATORS] = { };

	FMVoice(int note, float velocity, int start_time) : Voice(note, velocity, start_time) { }
};

struct FMComponent : VoiceComponent<FMVoice> {
	float weights[FM_NUM_OPERATORS][FM_NUM_OPERATORS] = { };
	float outs[FM_NUM_OPERATORS] = { 1.0f };

	Parameter<float> ratios[FM_NUM_OPERATORS] = {
		{ this, "ratio_0", "Rat", "Ratio", 1.0f, std::make_pair(0.25f, 16.0f), { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f } },
		{ this, "ratio_1", "Rat", "Ratio", 2.0f, std::make_pair(0.25f, 16.0f), { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f } },
		{ this, "ratio_2", "Rat", "Ratio", 4.0f, std::make_pair(0.25f, 16.0f), { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f } },
		{ this, "ratio_3", "Rat", "Ratio", 8.0f, std::make_pair(0.25f, 16.0f), { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f } }
	};
	
	Parameter<float> attacks[FM_NUM_OPERATORS] = {
		Parameter<float>::make_attack(this, "attack_0"),
		Parameter<float>::make_attack(this, "attack_1"),
		Parameter<float>::make_attack(this, "attack_2"),
		Parameter<float>::make_attack(this, "attack_3")
	};
	Parameter<float> holds[FM_NUM_OPERATORS] = {
		Parameter<float>::make_hold(this, "hold_0"),
		Parameter<float>::make_hold(this, "hold_1"),
		Parameter<float>::make_hold(this, "hold_2"),
		Parameter<float>::make_hold(this, "hold_3")
	};
	Parameter<float> decays[FM_NUM_OPERATORS] = {
		Parameter<float>::make_decay(this, "decay_0"),
		Parameter<float>::make_decay(this, "decay_1"),
		Parameter<float>::make_decay(this, "decay_2"),
		Parameter<float>::make_decay(this, "decay_3")
	};
	Parameter<float> sustains[FM_NUM_OPERATORS] = {
		Parameter<float>::make_sustain(this, "sustain_0"),
		Parameter<float>::make_sustain(this, "sustain_1"),
		Parameter<float>::make_sustain(this, "sustain_2"),
		Parameter<float>::make_sustain(this, "sustain_3")
	};

	Parameter<float> release = Parameter<float>::make_release(this);

	FMComponent(int id) : VoiceComponent(id, "FM", { { this, "MIDI In", true } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
	
	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;
};
