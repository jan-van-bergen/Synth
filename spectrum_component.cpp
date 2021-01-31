#include "components.h"

#include <complex>
#include <array>

#include "scope_timer.h"

static float hamming_window(int i) {
	return 0.53836f - 0.46164f * std::cos(TWO_PI * BLOCK_SIZE_INV * float(i));
}

static auto hamming_lut = util::generate_lookup_table<float, BLOCK_SIZE>(hamming_window);

void SpectrumComponent::update(Synth const& synth) {
	Sample fourier[N] = { };
	
	for (int i = 0; i < BLOCK_SIZE; i++) {
		fourier[i] = hamming_lut[i] * inputs[0].get_value(i);
	}

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

	float magnitudes[N] = { };

	for (int i = 0; i < N; i++) {
		magnitudes[i] = std::sqrt((fourier[i].left * fourier[i].left + fourier[i].right * fourier[i].right) * BLOCK_SIZE_INV);
	}

	float t = 0.0f;
	for (int i = 0; i < N; i++) {
		auto f = util::log_interpolate(1.0f, 0.5f * float(N), t);

		// Linear interpolation
		auto index_a = util::round(f - 0.5f);
		auto index_b = std::min(index_a + 1, N - 1);

		auto magnitude = util::lerp(magnitudes[index_a], magnitudes[index_b], f - std::floor(f));

		// Use Exponential Moving Average to temporally smooth out the Spectrum
		spectrum[i] = util::lerp(spectrum[i], magnitude, 0.2f);

		t += 1.0f / N;
	}
}

void SpectrumComponent::render(Synth const & synth) {
	auto avail = ImGui::GetContentRegionAvail();
	ImGui::PlotLines("", spectrum, N, 0, nullptr, 0.0f, 1.0f, ImVec2(avail.x, avail.y - ImGui::GetTextLineHeightWithSpacing()));
}
