#pragma once
#include "component.h"

struct EqualizerComponent : Component {
	static constexpr auto NUM_BANDS = 4;

	struct Band {
		static constexpr auto MAX_Q = 10.0f;

		static constexpr char const * mode_names[] = {
			"Low Pass",
			"Band Pass",
			"High Pass",
			"Peak",
			"Notch",
			"Low Shelf",
			"High Shelf"
		};
		int mode = 3;

		float freq;
		float Q    = 1.0f;
		float gain = 0.0f;

		dsp::BiQuadFilter<Sample> filter;

		Band(float freq) : freq(freq) { }
	};

	Band bands[NUM_BANDS] = { 63.0f, 294.0f, 1363.0f, 6324.0f };

	EqualizerComponent(int id) : Component(id, "Equalizer", { { this, "In" } }, { { this, "Out" } }) { }

	void update(struct Synth const & synth) override;
	void render(struct Synth const & synth) override;

	void   serialize_custom(json::Writer & writer) const override;
	void deserialize_custom(json::Object const & object) override;
};
