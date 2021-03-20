#include <vector>

#include "util/util.h"

namespace dsp {
	template<typename T>
	struct AllPassFilter {
		std::vector<T> history;
		int            history_offset = 0;

		float feedback = 0.0f;
		
		void set_delay(int delay_in_samples) {
			history.resize(delay_in_samples);
			history_offset %= history.size();
		}

		T process(T const & x) {
			auto old = history[history_offset];

			auto y = -x + old;

			history[history_offset] = x + feedback * old;
			history_offset = util::wrap<int>(history_offset + 1, history.size());

			return y;
		}
	};
}
