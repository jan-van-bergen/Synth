#pragma once

#include "synth/sample.h"
#include "util/util.h"

namespace dsp {
	// Based on: https://gist.github.com/agrafix/aa49c17cd32c8ba63b6a7cb8dce8b0bd
	template<int N, typename T>
	void fft(T fourier[]) {
		static_assert(util::is_power_of_two(N), "FFT can only handle N that are a power of two");

		// Reverse bits
		auto i2 = N / 2;
		auto j = 0;
		for (int i = 0; i < N - 1; i++) {
			if (i < j) {
				std::swap(fourier[i], fourier[j]);
			}

			auto k = i2;
			while (k <= j) {
				j -= k;
				k /= 2;
			}
			j += k;
		}

		// Fourier Transform
		auto c  = Sample(-1.0f, 0.0f);
		auto l2 = 1;

		for (int l = 0; l < util::log2(N); l++) {
			auto l1 = l2;
			l2 <<= 1;
			auto u1 = 1.0f;
			auto u2 = 0.0f;

			for (j = 0; j < l1; j++) {
				for (int i = j; i < N; i += l2) {
					auto i1 = i + l1;

					auto t1 = u1 * fourier[i1].left  - u2 * fourier[i1].right;
					auto t2 = u1 * fourier[i1].right + u2 * fourier[i1].left;

					fourier[i1].left  = fourier[i].left  - t1;
					fourier[i1].right = fourier[i].right - t2;
					fourier[i].left  += t1;
					fourier[i].right += t2;
				}

				auto z = u1 * c.left  - u2 * c.right;
				u2     = u1 * c.right + u2 * c.left;
				u1     = z;
			}

			c.right = -std::sqrt(0.5f - 0.5f * c.left);
			c.left  =  std::sqrt(0.5f + 0.5f * c.left);
		}
	}
}
