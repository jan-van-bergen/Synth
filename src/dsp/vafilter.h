#pragma once
#include "util/util.h"

namespace dsp {
	enum struct VAFilterMode {
		LOW_PASS,
		BAND_PASS,
		HIGH_PASS,
		OFF
	};

	template<typename T>
	struct VAFilter {
	private:
		VAFilterMode mode = VAFilterMode::LOW_PASS;

		float g         = 0.0f; // Gain
		float R         = 0.0f; // Damping
		float denom_inv = 0.0f;

		T state_1 = { };
		T state_2 = { };

	public:
		void set(VAFilterMode mode, float cutoff, float resonance) {
			this->mode = mode;

			g = std::tan(PI * cutoff * SAMPLE_RATE_INV);
			R = std::min(1.0f - resonance, 0.999f);
    
			denom_inv = 1.0f / (1.0f + (2.0f * R * g) + g * g);
		}

		void reset_state() {
			state_1 = { };
			state_2 = { };
		}

		T process(T const & sample) {
			if (mode == VAFilterMode::OFF) return sample;

			auto high_pass = (sample - (2.0f * R + g) * state_1 - state_2) * denom_inv;
			auto band_pass = high_pass * g + state_1;
			auto  low_pass = band_pass * g + state_2;
	
			state_1 = g * high_pass + band_pass;
			state_2 = g * band_pass + low_pass;

			switch (mode) {
				case VAFilterMode::LOW_PASS:  return low_pass;
				case VAFilterMode::BAND_PASS: return band_pass;
				case VAFilterMode::HIGH_PASS: return high_pass;

				default: abort();
			}
		}
	};
}
