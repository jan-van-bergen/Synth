#include "components.h"

#include <ImGui/implot.h>

void EqualizerComponent::update(Synth const & synth) {
	for (auto & band : bands) {
		dsp::BiQuadFilterMode mode;
		switch (band.mode) {
			case 0: mode = dsp::BiQuadFilterMode::LOW_PASS;   break;
			case 1: mode = dsp::BiQuadFilterMode::BAND_PASS;  break;
			case 2: mode = dsp::BiQuadFilterMode::HIGH_PASS;  break;
			case 3: mode = dsp::BiQuadFilterMode::PEAK;       break;
			case 4: mode = dsp::BiQuadFilterMode::NOTCH;      break;
			case 5: mode = dsp::BiQuadFilterMode::LOW_SHELF;  break;
			case 6: mode = dsp::BiQuadFilterMode::HIGH_SHELF; break;

			default: abort();
		}

		band.filter.set(mode, band.freq, band.Q, band.gain);	
	}
	
	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_sample(i);

		for (auto & band : bands) {
			sample = band.filter.process(sample);
		}

		outputs[0].set_sample(i, sample);
	}
}

void EqualizerComponent::render(Synth const & synth) {
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

	ImPlot::SetNextPlotLimits(20.0, 20000.0, -18.0, 18.0, ImGuiCond_Always);
	ImPlot::SetNextPlotTicksX(ticks_x, util::array_element_count(ticks_x), tick_labels);

	if (ImPlot::BeginPlot("EQ", "Frequency (Hz)", "Magnitude (dB)", space, ImPlotFlags_CanvasOnly, ImPlotAxisFlags_LogScale)) {
		static constexpr auto N = 4 * 1024;

		auto getter = [](void * data, int index) -> ImPlotPoint {
			auto freq = util::log_interpolate(20.0f, 20000.0f, (index + 0.5f) / N);
			
			auto sqrt_phi = std::sin(PI * freq * SAMPLE_RATE_INV);
			auto      phi = sqrt_phi * sqrt_phi;

			auto response = 0.0f;

			for (int b = 0; b < NUM_BANDS; b++) {
				auto const & band = reinterpret_cast<Band const *>(data)[b];

				auto b0 = band.filter.b0, b1 = band.filter.b1, b2 = band.filter.b2, a1 = band.filter.a1, a2 = band.filter.a2;

				// Single precision is sufficient, using the trick described here: https://dsp.stackexchange.com/a/16911
				auto b012 = 0.5f * (b0   + b1 + b2);
				auto a012 = 0.5f * (1.0f + a1 + a2);

				auto numerator   = b012*b012 - phi * (4.0f*b0*b2*(1.0f - phi) + b1*(b0   + b2));
				auto denominator = a012*a012 - phi * (4.0f   *a2*(1.0f - phi) + a1*(1.0f + a2));

				response += 10.0f * (std::log10(numerator) - std::log10(denominator));
			}

			return { freq, response };
		};

		ImPlot::PlotLineG("", getter, bands, N);

		char label[32] = { };
		char popup_label[32] = { };

		for (int b = 0; b < NUM_BANDS; b++) {
			auto & band = bands[b];

			sprintf_s(label, "#%i", b);
			sprintf_s(popup_label, "Popup #%i", b);
			
			auto f = double(band.freq);
			auto g = double(band.gain);

			ImPlot::DragPoint(label, &f, &g);
			
			band.freq = f;
			band.gain = g;

			if (ImGui::IsItemHovered()) {
				band.Q = util::clamp(band.Q - 0.1f * ImGui::GetIO().MouseWheel, 0.0f, Band::MAX_Q);

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup(popup_label);
				}
			}
			
			if (ImGui::BeginPopup(popup_label)) {
				if (ImGui::BeginCombo("Mode", Band::mode_names[band.mode])) {
					for (int j = 0; j < util::array_element_count(Band::mode_names); j++) {
						if (ImGui::Selectable(Band::mode_names[j], band.mode == j)) {
							band.mode = j;
						}
					}

					ImGui::EndCombo();
				}

				ImGui::Text("Freq: %.1f", band.freq);
				ImGui::Text("Q:    %.1f", band.Q);
				ImGui::Text("gain: %.1f", band.gain);

				ImGui::EndPopup();
			}

			ImPlot::PlotText(label, f, g, false, ImVec2(0, -12));
		}

		ImPlot::EndPlot();
	}
}

void EqualizerComponent::serialize_custom(json::Writer & writer) const {
	writer.write("NUM_BANDS", NUM_BANDS);

	char label[32] = { };

	for (int i = 0; i < NUM_BANDS; i++) {
		auto const & band = bands[i];

		sprintf_s(label, "Band_%i", i);
		writer.object_begin(label);

		writer.write("mode", band.mode);
		writer.write("freq", band.freq);
		writer.write("Q",    band.Q);
		writer.write("gain", band.gain);
		
		writer.object_end();
	};
}

void EqualizerComponent::deserialize_custom(json::Object const & object) {
	auto num_bands = object.find_int("NUM_BANDS");

	if (num_bands != NUM_BANDS) {
		printf("WARNING: NUM_BANDS of Equalizer did not match!\n");
		__debugbreak();
	}

	char label[32] = { };

	for (int i = 0; i < NUM_BANDS; i++) {
		sprintf_s(label, "Band_%i", i);
		auto band_obj = object.find<json::Object const>(label);

		if (band_obj == nullptr) {
			bands[i].mode = 3;
			bands[i].freq = 1000.0f;
			bands[i].Q    = 1.0f;
			bands[i].gain = 0.0f;
		} else {
			bands[i].mode = band_obj->find_int  ("mode", 3);
			bands[i].freq = band_obj->find_float("freq", 1000.0f);
			bands[i].Q    = band_obj->find_float("Q",    1.0f);
			bands[i].gain = band_obj->find_float("gain", 0.0f);
		}
	}
}
