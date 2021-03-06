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

	enum struct BiQuadFilterMode {
		LOW_PASS,
		BAND_PASS,
		HIGH_PASS,
		ALL_PASS,
		PEAK,
		NOTCH,
		LOW_SHELF,
		HIGH_SHELF
	};

	// Based on: https://arachnoid.com/BiQuadDesigner/index.html
	template<typename T>
	struct BiQuadFilter {
		BiQuadFilterMode mode = BiQuadFilterMode::LOW_PASS;

		float b0 = 1.0f, b1 = 1.0f, b2 = 1.0f, a1 = 1.0f, a2 = 1.0f;

		T x1 = { }, x2 = { };
		T y1 = { }, y2 = { };

		void set(BiQuadFilterMode mode, float frequency, float Q, float gain_db = 0.0f) {
			this->mode = mode;

			auto omega = TWO_PI * frequency * SAMPLE_RATE_INV;
			auto sin_omega = std::sin(omega);
			auto cos_omega = std::cos(omega);

			auto gain = util::db_to_linear(0.5f * gain_db);
			
			auto alpha = sin_omega / (2.0f * std::max(Q, 0.001f));
			auto beta  = std::sqrt(2.0f * gain);
			
			auto a0 = 1.0f;

			switch (mode) {
				case BiQuadFilterMode::LOW_PASS: {
					b0 = (1.0f - cos_omega) * 0.5f;
					b1 = (1.0f - cos_omega);
					b2 = (1.0f - cos_omega) * 0.5f;
					a0 =  1.0f + alpha;
					a1 = -2.0f * cos_omega;
					a2 =  1.0f - alpha;
					break;
				}
				case BiQuadFilterMode::BAND_PASS: {
					b0 = alpha;
					b1 = 0.0f;
					b2 = -alpha;
					a0 =  1.0f + alpha;
					a1 = -2.0f * cos_omega;
					a2 =  1.0f - alpha;
					break;
				}
				case BiQuadFilterMode::HIGH_PASS: {
					b0 =  (1.0f + cos_omega) * 0.5f;
					b1 = -(1.0f + cos_omega);
					b2 =  (1.0f + cos_omega) * 0.5f;
					a0 =   1.0f + alpha;
					a1 =  -2.0f * cos_omega;
					a2 =   1.0f - alpha;
					break;
				}
				case BiQuadFilterMode::ALL_PASS: {			
					b0 =  1.0f - alpha;
					b1 = -2.0f * cos_omega;
					b2 =  1.0f + alpha;
					a0 =  1.0f + alpha;
					a1 = -2.0f * cos_omega;
					a2 =  1.0f - alpha;
					break;
				}
				case BiQuadFilterMode::PEAK: {
					b0 =  1.0f + (alpha * gain);
					b1 = -2.0f * cos_omega;
					b2 =  1.0f - (alpha * gain);
					a0 =  1.0f + (alpha / gain);
					a1 = -2.0f * cos_omega;
					a2 =  1.0f - (alpha / gain);
					break;
				}
				case BiQuadFilterMode::NOTCH: {
					b0 =  1.0f;
					b1 = -2.0f * cos_omega;
					b2 =  1.0f;
					a0 =  1.0f + alpha;
					a1 = -2.0f * cos_omega;
					a2 =  1.0f - alpha;
					break;
				}
				case BiQuadFilterMode::LOW_SHELF: {
					b0 =         gain * ((gain + 1.0f) - (gain - 1.0f) * cos_omega + beta * sin_omega);
					b1 =  2.0f * gain * ((gain - 1.0f) - (gain + 1.0f) * cos_omega);
					b2 =         gain * ((gain + 1.0f) - (gain - 1.0f) * cos_omega - beta * sin_omega);
					a0 =                 (gain + 1.0f) + (gain - 1.0f) * cos_omega + beta * sin_omega;
					a1 = -2.0f        * ((gain - 1.0f) + (gain + 1.0f) * cos_omega);
					a2 =                 (gain + 1.0f) + (gain - 1.0f) * cos_omega - beta * sin_omega;
					break;
				}
				case BiQuadFilterMode::HIGH_SHELF: {
					b0 =         gain * ((gain + 1.0f) + (gain - 1.0f) * cos_omega + beta * sin_omega);
					b1 = -2.0f * gain * ((gain - 1.0f) + (gain + 1.0f) * cos_omega);
					b2 =         gain * ((gain + 1.0f) + (gain - 1.0f) * cos_omega - beta * sin_omega);
					a0 =                 (gain + 1.0f) - (gain - 1.0f) * cos_omega + beta * sin_omega;
					a1 =  2.0f        * ((gain - 1.0f) - (gain + 1.0f) * cos_omega);
					a2 =                 (gain + 1.0f) - (gain - 1.0f) * cos_omega - beta * sin_omega;
					break;
				}

				default: abort();
			}
			
			// Normalize
			b0 /= a0;
			b1 /= a0;
			b2 /= a0;
			a1 /= a0;
			a2 /= a0;
		}

		void reset_state() {
			x1 = { };
			x2 = { };
			y1 = { };
			y2 = { };
		}

		T process(T const & x) {
			auto y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
			
			x2 = x1;
			x1 = x;
			y2 = y1;
			y1 = y;

			return y;
		}
	};
}
