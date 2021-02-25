#pragma once
#include <ImGui/imgui.h>

#include "util.h"

namespace ImGui {
	bool Knob(char const * label, char const * name_display, char const * name_full, float * value, float min, float max, bool log_scale = false, util::Formatter<float> formatter = nullptr, float size = 48.0f);
	bool Knob(char const * label, char const * name_display, char const * name_full, int   * value, int   min, int   max, bool log_scale = false, util::Formatter<int>   formatter = nullptr, float size = 48.0f);
}
