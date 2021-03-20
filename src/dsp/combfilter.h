#pragma once
#include "util/util.h"

namespace dsp {
	template<typename T>
	struct CombFilter {
		std::vector<T> history;
		int            history_offset = 0;

		float feedback = 0.0f;
		float damp     = 0.0f;

	private:
		T state = { };

	public:
		void set_delay(int delay_in_samples) {
			history.resize(delay_in_samples);
			history_offset %= history.size();
		}

		T process(T const & x) {
			auto y = history[history_offset];
			state = util::lerp(y, state, damp);

			history[history_offset] = x + feedback * state;
			history_offset = util::wrap<int>(history_offset + 1, history.size());

			return y;
		}
	};
}
