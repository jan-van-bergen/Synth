#pragma once
#include "component.h"

#include "dsp/biquadfilter.h"

struct VocoderComponent : Component {
	struct Band {
		float freq = 1000.0f;

		dsp::BiQuadFilter<Sample> filter_mod;
		dsp::BiQuadFilter<Sample> filter_car;

		float gain = 0.0f;
	};
	
	std::vector<Band> bands;

	Parameter<int> num_bands = { this, "num_bands", "Bands", "Number of Bands", 16, std::make_pair(1, 64) };

	Parameter<float> width = { this, "width", "Q",         "Width", 5.0f, std::make_pair(0.0f, 10.0f) };
	Parameter<float> decay = { this, "decay", "Dec",       "Decay", 0.5f, std::make_pair(0.0f,  1.0f) };
	Parameter<float> gain  = { this, "gain",  "Gain (dB)", "Gain",  0.0f, std::make_pair(0.0f, 30.0f) };
	
	VocoderComponent(int id) : Component(id, "Vocoder", { { this, "Modulator" }, { this, "Carrier" } }, { { this, "Out" } }) {
		calc_bands();
	}

	void calc_bands();

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void deserialize_custom(json::Object const & object) override;
};
