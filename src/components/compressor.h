#pragma once
#include "component.h"

struct CompressorComponent : Component {
	Parameter<float> threshold = { this, "threshold", "Thrs", "Threshold (dB)", 0.0f, std::make_pair(-60.0f,   0.0f) };
	Parameter<float> ratio     = { this, "ratio",     "Rat",  "Ratio",          1.0f, std::make_pair(  1.0f,  30.0f) };
	Parameter<float> gain      = { this, "gain",      "Gain", "Gain (dB)",      0.0f, std::make_pair(-30.0f,  30.0f) };
	Parameter<float> attack    = { this, "Attack",    "Att",  "Attack (ms)",   15.0f, std::make_pair(  0.0f, 400.0f) };
	Parameter<float> release   = { this, "Release",   "Rel",  "Release (ms)", 200.0f, std::make_pair(  0.0f, 400.0f) };

	CompressorComponent(int id) : Component(id, "Compressor", { { this, "Input" }, { this, "Sidechain" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	float env = 0.0f;
};
