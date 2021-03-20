#include "component.h"

#include "dsp/allpass_filter.h"
#include "dsp/combfilter.h"

struct ReverbComponent : Component {
private:
	static constexpr auto MAX_SPREAD = 100; // How many samples can the right channel at most delay behind the left channel

	static constexpr auto NUM_COMB_FILTERS    = 8;
	static constexpr auto NUM_ALLPASS_FILTERS = 4;

	dsp::CombFilter<float> comb_filters_left [NUM_COMB_FILTERS];
	dsp::CombFilter<float> comb_filters_right[NUM_COMB_FILTERS];

	dsp::AllPassFilter<float> allpass_filters_left [NUM_ALLPASS_FILTERS];
	dsp::AllPassFilter<float> allpass_filters_right[NUM_ALLPASS_FILTERS];

public:
	Parameter<float> room   = { this, "room",   "Roo", "Room Size",      1.0f, std::make_pair(0.1f, 10.0f), { 1.0f } };
	Parameter<float> decay  = { this, "decay",  "Dec", "Decay",          1.0f, std::make_pair(0.1f, 2.0f) };
	Parameter<float> spread = { this, "spread", "Spd", "Stereo Spread",  0.2f, std::make_pair(0.0f, 1.0f) };
	Parameter<float> damp   = { this, "damp",   "Dmp", "Damping",        0.2f, std::make_pair(0.0f, 1.0f) };
	Parameter<float> dry    = { this, "dry",    "Dry", "Dry Level (dB)", 0.0f, std::make_pair(-60.0f, 0.0f) };
	Parameter<float> wet    = { this, "wet",    "Wet", "Wet Level (dB)", 0.0f, std::make_pair(-60.0f, 0.0f) };

	ReverbComponent(int id) : Component(id, "Reverb", { { this, "In" } }, { { this, "Out" } }) {
		calc_comb_filter_delays();
	}

	void calc_comb_filter_delays();
	void calc_comb_filter_feedbacks();

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;
};
