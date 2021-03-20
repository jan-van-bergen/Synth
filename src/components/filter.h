#pragma once
#include "component.h"

#include "dsp/vafilter.h"

struct FilterComponent : Component {
	Parameter<int> filter_type = { this, "filter_type", "Type", "Filter Type", 0, std::make_pair(0, 3) };

	Parameter<float> cutoff    = { this, "cutoff",    "Freq", "Cutoff Frequency", 1000.0f, std::make_pair(20.0f, 20000.0f), { 50, 100, 200, 400, 800, 1600, 3200, 6400, 12800, 19200 }, Param::Curve::LOGARITHMIC };
	Parameter<float> resonance = { this, "resonance", "Res",  "Resonance",           0.5f, std::make_pair(0.0f, 1.0f) };

	FilterComponent(int id) : Component(id, "Filter", { { this, "In" } }, { { this, "Out" } }) { }
	
	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

private:
	dsp::VAFilter<Sample> filter;
};
