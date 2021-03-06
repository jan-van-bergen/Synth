#pragma once

namespace ImGui {
	bool Knob(char const * label, char const * name_display, char const * name_full, float * value, float min, float max, bool log_scale = false, char const * fmt = "%.2f", float size = 48.0f);
	bool Knob(char const * label, char const * name_display, char const * name_full, int   * value, int   min, int   max, bool log_scale = false, char const * fmt = "%i",   float size = 48.0f);
}
