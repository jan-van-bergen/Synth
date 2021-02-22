#include "components.h"

#include <complex>
#include <array>

#include <ImGui/implot.h>

#include "scope_timer.h"

static float hamming_window(int i) {
	return 0.53836f - 0.46164f * std::cos(TWO_PI * BLOCK_SIZE_INV * float(i));
}

static auto const hamming_lut = util::generate_lookup_table<float, BLOCK_SIZE>(hamming_window);

void SpectrumComponent::update(Synth const& synth) {
	Sample fourier[N] = { };
	
	for (int i = 0; i < BLOCK_SIZE; i++) {
		fourier[i] = hamming_lut[i] * inputs[0].get_sample(i);
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

	for (int i = 0; i < N / 2; i++) {
		auto magnitude = std::sqrt(fourier[i].left * fourier[i].left + fourier[i].right * fourier[i].right) / N;
		magnitudes[i] = util::lerp(magnitudes[i], magnitude, 0.1f);
	}
}

void SpectrumComponent::render(Synth const & synth) {
	static constexpr auto FREQ_BIN_SIZE = SAMPLE_RATE / float(N);

	static constexpr double ticks_x[] = {
		20.0,   30.0,   40.0,   60.0,   80.0,   100.0,
		200.0,  300.0,  400.0,  600.0,  800.0,  1000.0,
		2000.0, 3000.0, 4000.0, 6000.0, 8000.0, 10000.0,
		20000.0
	};
	static constexpr char const * tick_labels[] = {
		"20",  "30",  "40",  "60",  "80",  "100",	
		"200", "300", "400", "600", "800", "1000",	
		"2K",  "3K",  "4K",  "6K",  "8K",  "10K",
		"20K"
	};
	
	auto avail = ImGui::GetContentRegionAvail();
	auto space = ImVec2(avail.x, std::max(
		ImGui::GetTextLineHeightWithSpacing(),
		avail.y - (inputs.size() + outputs.size()) * ImGui::GetTextLineHeightWithSpacing()
	));

	ImPlot::SetNextPlotLimits(FREQ_BIN_SIZE, SAMPLE_RATE / 2, -78.0f, -18.0f, ImGuiCond_Always);
	ImPlot::SetNextPlotTicksX(ticks_x, util::array_element_count(ticks_x), tick_labels);

	ImPlot::BeginPlot("Spectrum", "Frequency (Hz)", "Magnitude (dB)", space, ImPlotFlags_CanvasOnly, ImPlotAxisFlags_LogScale);
	ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

	// Convert magnitude to dB
	// Do this once for both plots, instead of using a getter
	float spectrum[N / 2] = { };
	for (int i = 0; i < N / 2; i++) spectrum[i] = util::linear_to_db(magnitudes[i]);
	
	ImPlot::PlotShaded("", spectrum, N / 2, -INFINITY, FREQ_BIN_SIZE, FREQ_BIN_SIZE);
	ImPlot::PlotLine  ("", spectrum, N / 2,            FREQ_BIN_SIZE, FREQ_BIN_SIZE);

	ImPlot::PopStyleVar();
	ImPlot::EndPlot();
}
