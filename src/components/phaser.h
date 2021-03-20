#pragma once
#include "component.h"

#include "dsp/biquadfilter.h"

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
