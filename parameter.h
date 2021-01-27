#pragma once
#include <string>
#include <vector>
#include <optional>

#include <ImGui/imgui.h>

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
	{ }

	void render() {
		auto [lower, upper] = bounds;

		char const * fmt = nullptr;

		if constexpr (std::is_same<T, float>()) {
			ImGui::SliderFloat(name.c_str(), &parameter, lower, upper);

			fmt = "%.3f";
		} else if constexpr (std::is_same<T, int>()) {
			ImGui::SliderInt(name.c_str(), &parameter, lower, upper);

			fmt = "%i";
		} else {
			abort();
		}

		if (options.size() > 0 && ImGui::BeginPopupContextItem()) {
			char label[128] = { };

			for (auto const & option : options) {
				sprintf_s(label, fmt, option);

				if (ImGui::Button(label)) parameter = option;
			}

			ImGui::EndPopup();
		}
	}

	operator T() const { return parameter; }
};
