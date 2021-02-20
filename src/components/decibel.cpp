#include "components.h"

void DecibelComponent::update(Synth const & synth) {
	auto max_amplitude = 0.0f;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		auto sample = inputs[0].get_sample(i);
		auto amplitude = std::max(std::abs(sample.left), std::abs(sample.right));

		max_amplitude = std::max(max_amplitude, amplitude);
	}

	decibels = util::linear_to_db(max_amplitude);
	if (decibels < -60.0f) decibels = -INFINITY;

	if (history.size() < HISTORY_LENGTH) {
		history.push_back(decibels);
	} else {
		history[history_index] = decibels;
		history_index = util::wrap(history_index + 1, HISTORY_LENGTH);
	}
}

void DecibelComponent::render(Synth const & synth) {
	static constexpr auto METER_WIDTH  = 36.0f;
	static constexpr auto METER_HEIGHT = 96.0f;

	auto const colour_background = ImColor( 50,  50,  50, 255);
	auto const colour_good       = ImColor( 50, 250,  50, 255);
	auto const colour_bad        = ImColor(250,  50,  50, 255);
	auto const colour_white      = ImColor(250, 250, 250, 255);

	auto colour_foreground = decibels < 0.0f ? colour_good : colour_bad;

	static constexpr auto DB_MIN = -60.0f;
	static constexpr auto DB_MAX =   0.0f;

	auto height_factor = util::remap(util::clamp(decibels, DB_MIN, DB_MAX), DB_MIN, DB_MAX, 0.0f, 1.0f);

	// Apply some temporal smoothing when volume goes down, not when it goes up
	if (height_factor < previous_height_factor) {
		height_factor = util::lerp(previous_height_factor, height_factor, 0.1f);
	}
	previous_height_factor = height_factor;
	
	auto window_pos = ImGui::GetWindowPos();
	auto cursor_pos = ImGui::GetCursorPos();

	auto x = window_pos.x + cursor_pos.x;
	auto y = window_pos.y + cursor_pos.y;
	
	auto draw_list = ImGui::GetWindowDrawList();

	draw_list->AddRectFilled(ImVec2(x, y),                                         ImVec2(x + METER_WIDTH, y + METER_HEIGHT), colour_background);
	draw_list->AddRectFilled(ImVec2(x, y + (1.0f - height_factor) * METER_HEIGHT), ImVec2(x + METER_WIDTH, y + METER_HEIGHT), colour_foreground);
	
	auto text_height = ImGui::GetTextLineHeight();

	draw_list->AddText(ImVec2(x + METER_WIDTH, y),                              colour_white,   "0 dB");
	draw_list->AddText(ImVec2(x + METER_WIDTH, y + METER_HEIGHT - text_height), colour_white, "-60 dB");

	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::NewLine();

	auto avg = 0.0f;
	auto min = +INFINITY;
	auto max = -INFINITY;

	for (auto db : history) {
		avg += db;
		min = std::min(db, min);
		max = std::max(db, max);
	}

	avg /= float(history.size());

	ImGui::Text("Cur: %.2f dB", decibels);
	ImGui::Text("Avg: %.2f dB", avg);
	ImGui::Text("Min: %.2f dB", min);
	ImGui::Text("Max: %.2f dB", max);
}
