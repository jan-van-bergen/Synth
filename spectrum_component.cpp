#include "components.h"

#include <complex>

static float hamming_window(int i) {
	static constexpr auto h = TWO_PI * BLOCK_SIZE_INV;

	return 0.53836f - 0.46164f * std::cos(h * float(i));
}

void SpectrumComponent::update(Synth const& synth) {
	Sample fourier[BLOCK_SIZE];

	for (int i = 0; i < BLOCK_SIZE; i++) {
		fourier[i] = hamming_window(i) * inputs[0].get_value(i);
	}

	static constexpr auto      N = BLOCK_SIZE;
	static constexpr auto log2_N = [](int n) { auto log = 0; while (n > 1) { n /= 2; log ++; } return log; }(N);

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

	for (int l = 0; l < log2_N; l++) {
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

	float magnitudes[BLOCK_SIZE] = { };

	for (int i = 0; i < BLOCK_SIZE; i++) {
		magnitudes[i] = std::sqrt((fourier[i].left * fourier[i].left + fourier[i].right * fourier[i].right) * BLOCK_SIZE_INV);
	}

	float t = 0.0f;
	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto f = util::log_interpolate(1.0f, float(BLOCK_SIZE), t);

		// Linear interpolation
		auto index_a = util::float_to_int(f);
		auto index_b = std::min(index_a + 1, BLOCK_SIZE - 1);

		auto magnitude = util::lerp(magnitudes[index_a], magnitudes[index_b], f - std::floor(f));

		// Use Exponential Moving Average to temporally smooth out the Spectrum
		spectrum[i] = util::lerp(spectrum[i], magnitude, 0.2f);

		t += BLOCK_SIZE_INV;
	}
}

void SpectrumComponent::render(Synth const & synth) {
	auto avail = ImGui::GetContentRegionAvail();
	ImGui::PlotLines("", spectrum, BLOCK_SIZE, 0, nullptr, 0.0f, 1.0f, ImVec2(avail.x, avail.y - ImGui::GetTextLineHeightWithSpacing()));
}
