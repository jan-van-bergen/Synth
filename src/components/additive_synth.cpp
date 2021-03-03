#include "components.h"

#include "synth.h"

void AdditiveSynthComponent::update(Synth const & synth) {
	auto steps_per_second = 4.0f / 60.0f * float(synth.settings.tempo);
	
	auto note_events = inputs[0].get_events();

	for (auto const & note_event : note_events) {
		if (note_event.pressed) {
			voices.emplace_back(note_event.note, note_event.velocity);
		} else {
			voices.erase(std::remove_if(voices.begin(), voices.end(), [note = note_event.note](auto voice) {
				return voice.note == note;
			}), voices.end());
		}
	}
	
	for (auto & voice : voices) {
		auto frequency = util::note_freq(voice.note);

		for (int i = 0; i < BLOCK_SIZE; i++) {
			auto time_in_seconds = voice.sample * SAMPLE_RATE_INV;
			auto time_in_steps   = time_in_seconds * steps_per_second;

			auto phase = TWO_PI * time_in_seconds * frequency;
		
			Sample sample = { };

			auto harmonic_multiplier = 1.0f;

			for (auto harmonic : harmonics) {
				sample += harmonic * std::sin(harmonic_multiplier * phase);

				harmonic_multiplier += 1.0f;
			}

			outputs[0].get_sample(i) += voice.velocity * sample;

			voice.sample += 1.0f;
		}
	}
}

void AdditiveSynthComponent::render(Synth const & synth) {
	static constexpr auto HARMONIC_WIDTH   =  8.0f;
	static constexpr auto HARMONIC_HEIGHT  = 64.0f;
	static constexpr auto HARMONIC_PADDING =  2.0f;

	auto window = ImGui::GetWindowPos();
	auto cursor = ImGui::GetCursorPos();
	auto cur = ImVec2(window.x + cursor.x, window.y + cursor.y);
	
	auto mouse_pos  = ImGui::GetMousePos();
	auto mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);

	auto const & style = ImGui::GetStyle();
	auto colour_bg = ImColor(style.Colors[ImGuiCol_FrameBg]);
	auto colour_fg = ImColor(style.Colors[ImGuiCol_SliderGrab]);
	
	// Add invisible button to allow ImGui to resize the window to the appropriate size
	ImGui::InvisibleButton("Harmonics Region", ImVec2(NUM_HARMONICS * (HARMONIC_WIDTH + HARMONIC_PADDING), HARMONIC_HEIGHT));

	// Use the invisible button to check if the current Window has focus, ignore mouse if not
	if (!ImGui::IsItemHovered()) mouse_down = false;

	auto draw_list = ImGui::GetWindowDrawList();

	for (auto & harmonic : harmonics) {
		auto pos_min = ImVec2(cur.x,                  cur.y);
		auto pos_max = ImVec2(cur.x + HARMONIC_WIDTH, cur.y + HARMONIC_HEIGHT);
		auto pos_val = ImVec2(cur.x,                  cur.y + HARMONIC_HEIGHT * (1.0f - harmonic));

		draw_list->AddRectFilled(pos_min, pos_max, colour_bg);
		draw_list->AddRectFilled(pos_val, pos_max, colour_fg);

		if (mouse_down) {
			auto rect_contains_mouse =
				mouse_pos.x >= pos_min.x && mouse_pos.x < pos_max.x &&
				mouse_pos.y >= pos_min.y && mouse_pos.y < pos_max.y;

			if (rect_contains_mouse) harmonic = util::remap(mouse_pos.y, pos_min.y, pos_max.y, 1.0f, 0.0f);
		}

		cur.x += HARMONIC_WIDTH + HARMONIC_PADDING;
	}

	if (ImGui::Button("Saw")) {
		for (int i = 0; i < NUM_HARMONICS; i++) {
			harmonics[i] = 1.0f / (i + 1.0f);
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Square")) {
		for (int i = 0; i < NUM_HARMONICS; i++) {
			if (i % 2 == 0) {
				harmonics[i] = 1.0f / (i + 1.0f);
			} else {
				harmonics[i] = 0.0f;
			}
		}
	}
}

void AdditiveSynthComponent::serialize_custom(json::Writer & writer) const {
	writer.write("harmonics", NUM_HARMONICS, harmonics);
}

void AdditiveSynthComponent::deserialize_custom(json::Object const & object) {
	auto json_harmonics = object.find<json::Array const>("harmonics");
	
	if (!json_harmonics || json_harmonics->values.size() != NUM_HARMONICS) {
		puts("ERROR: Invalid harmonics!");
		return;
	}
	
	auto harmonic = harmonics;
	
	for (auto const & json_value : json_harmonics->values) {
		assert(json_value->type == json::JSON::Type::VALUE_FLOAT);
		auto value = static_cast<json::ValueFloat const *>(json_value.get());

		*(harmonic++) = value->value;
	}
}
