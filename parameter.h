#pragma once
#include <string>
#include <vector>
#include <optional>

#include <ImGui/imgui.h>

#include "util.h"

inline static char parameter_clipboard[4] = { };

template<typename T>
struct Parameter {
	static_assert(std::is_floating_point<T>() || std::is_integral<T>());

	std::string name;

	T parameter;

	std::pair<T, T> bounds;

	std::vector<T> options; // Options available in context menu

	Parameter(std::string const & name, T initial, std::pair<T, T> const & bounds, std::vector<T> options = { }) : 
		name(name), 
		parameter(initial), 
		bounds(bounds), 
		options(options)
	{ 
		for (auto const & option : options) assert(bounds.first <= option <= bounds.second);
	}

	void render() {
		auto [lower, upper] = bounds;

		char const * fmt = nullptr;
		
		float scroll_speed;

		// Render slider
		if constexpr (std::is_same<T, float>()) {
			ImGui::SliderFloat(name.c_str(), &parameter, lower, upper);

			fmt = "%.3f";

			scroll_speed = 0.01f;
		} else if constexpr (std::is_same<T, int>()) {
			ImGui::SliderInt(name.c_str(), &parameter, lower, upper);

			fmt = "%i";

			scroll_speed = 1.0f;
		} else {
			abort();
		}

		if (ImGui::IsItemHovered()) {
			parameter = util::clamp(parameter + ImGui::GetIO().MouseWheel * scroll_speed, lower, upper);
		}

		// Render default options context menu
		if (ImGui::BeginPopupContextItem()) {
			char label[128] = { };

			for (auto const & option : options) {
				sprintf_s(label, fmt, option);

				if (ImGui::Button(label)) parameter = option;
			}

			if (options.size() > 0) ImGui::Separator();

			if (ImGui::Button("Copy")) {
				memcpy(parameter_clipboard, &parameter, 4);
			}
			if (ImGui::Button("Paste")) {
				memcpy(&parameter, parameter_clipboard, 4);

				parameter = util::clamp(parameter, lower, upper);
			}

			ImGui::EndPopup();
		}
	}

	operator T() const { return parameter; }
};
