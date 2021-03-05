#pragma once
#include "component.h"

#include "dsp/filter.h"

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
