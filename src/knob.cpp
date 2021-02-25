#include "knob.h"

template<typename T>
bool knob(char const * label, char const * name_display, char const * name_full, T * value, T min, T max, bool log_scale, util::Formatter<T> formatter, float size) {
	static constexpr auto IS_FLOAT = std::is_same<T, float>();
	static constexpr auto IS_INT   = std::is_same<T, int>();

	static_assert(IS_FLOAT || IS_INT);
	
	float scroll_speed;
	float drag_speed;
	
	char fmt[128] = { };
	if (formatter) {
		formatter(*value, fmt, sizeof(fmt));
	}

	if constexpr (IS_FLOAT) {
		scroll_speed = ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) ? 0.001f : 0.01f; // Allow for fine scrolling using CONTROL key
		drag_speed   = scroll_speed;

		if (!formatter) strcpy_s(fmt, "%.2f");
	} else {
		scroll_speed = 1.0f / (max - min);
		drag_speed   = 0.01f;
		
		if (!formatter) strcpy_s(fmt, "%i");
	}
	
	auto window = ImGui::GetWindowPos();
	auto cursor = ImGui::GetCursorPos();
	ImGui::InvisibleButton(label, ImVec2(size, size));

	// Handle dragging and scrolling
	auto is_hovered = ImGui::IsItemHovered();
		
	auto id = ImGui::GetID(label);
	
	auto percent = log_scale ? 
		util::remap<float>(std::log(*value), std::log(min), std::log(max), 0.0f, 1.0f) :
		util::remap<float>(*value, min, max, 0.0f, 1.0f);

	struct {
		ImGuiID id;
		float original_mouse_y;
		float original_percentage;
	} static selected = { };

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		selected.id = 0;
	}

	if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		selected.id = id;
		selected.original_mouse_y = ImGui::GetMousePos().y;
		selected.original_percentage = percent;
	}
	
	auto value_changed = false;
	
	if (is_hovered && ImGui::GetIO().MouseWheel != 0.0f) {
		percent += ImGui::GetIO().MouseWheel * scroll_speed;

		value_changed = true;
	}

	if (id == selected.id) {
		auto diff = selected.original_mouse_y - ImGui::GetMousePos().y;
		percent = selected.original_percentage + drag_speed * diff;

		is_hovered    = true;
		value_changed = true;
	}
	
	if (value_changed) {
		percent = util::clamp(percent, 0.0f, 1.0f);

		float interpolated_value = log_scale ? 
			util::log_interpolate<float>(min, max, percent) :
			util::lerp<float>(min, max, percent);

		if constexpr (IS_FLOAT) {
			*value = interpolated_value;
		} else {
			*value = util::round(interpolated_value);
		}
	}

	// Display tooltip with full name
	auto name_is_non_empty = name_full && name_full[0] != '\0';

	if (is_hovered && name_is_non_empty) {
		ImGui::SetNextWindowBgAlpha(0.75f);

		ImGui::BeginTooltip();
		ImGui::TextUnformatted(name_full);
		ImGui::EndTooltip();
	}
	
	// Draw knob arcs
	auto center = ImVec2(
		window.x + cursor.x + 0.5f * size,
		window.y + cursor.y + 0.5f * size
	);
	
	auto const & style = ImGui::GetStyle();
	auto colour_bg   = ImColor(is_hovered ? style.Colors[ImGuiCol_FrameBgHovered]   : style.Colors[ImGuiCol_FrameBg]);
	auto colour_fg   = ImColor(is_hovered ? style.Colors[ImGuiCol_SliderGrabActive] : style.Colors[ImGuiCol_SliderGrab]);
	auto colour_text = ImColor(style.Colors[ImGuiCol_Text]);

	static constexpr auto ARC_RADIUS     = 0.9f;
	static constexpr auto ARC_ANGLE      = 0.7f * TWO_PI;
	static constexpr auto ARC_THICKNESS  = 5.0f;

	auto angle_min   =          0.5f * PI + 0.5f * (TWO_PI - ARC_ANGLE);
	auto angle_max   = TWO_PI + 0.5f * PI - 0.5f * (TWO_PI - ARC_ANGLE);
	auto angle_value = util::lerp(angle_min, angle_max, percent);
	
	auto draw_list = ImGui::GetWindowDrawList();
	
	draw_list->PathArcTo(center, ARC_RADIUS * 0.5f * size, angle_min, angle_max, 32);
	draw_list->PathStroke(colour_bg, false, ARC_THICKNESS);
	
	draw_list->PathArcTo(center, ARC_RADIUS * 0.5f * size, angle_min, angle_value, 32);
	draw_list->PathStroke(colour_fg, false, ARC_THICKNESS);

	// Draw text
	char value_str[128] = { };
	sprintf_s(value_str, fmt, *value);

	auto text_size_name  = ImGui::CalcTextSize(name_display);
	auto text_size_label = ImGui::CalcTextSize(value_str);
	
	if (name_display && name_display[0] != '\0') {
		draw_list->AddText(ImVec2(center.x - 0.5f * text_size_name .x, center.y - text_size_name.y), colour_text, name_display);
		draw_list->AddText(ImVec2(center.x - 0.5f * text_size_label.x, center.y),                    colour_text, value_str);
	} else {
		// If display name is empty, draw only the value
		draw_list->AddText(ImVec2(center.x - 0.5f * text_size_label.x, center.y - 0.5f * text_size_label.y), colour_text, value_str);
	}

	return value_changed;
}

bool ImGui::Knob(char const * label, char const * name_display, char const * name_full, float * value, float min, float max, bool log_scale, util::Formatter<float> formatter, float size) {
	return knob(label, name_display, name_full, value, min, max, log_scale, formatter, size);
}

bool ImGui::Knob(char const * label, char const * name_display, char const * name_full, int * value, int min, int max, bool log_scale, util::Formatter<int> formatter, float size) {
	return knob(label, name_display, name_full, value, min, max, log_scale, formatter, size);
}