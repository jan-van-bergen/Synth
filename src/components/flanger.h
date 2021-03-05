#pragma once
#include "component.h"

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
