#pragma once
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include <ImGui/imgui.h>

#include "util.h"

struct Param {
	inline static char clipboard[4] = { };

	inline static Param * param_waiting_to_link = nullptr;
	inline static std::unordered_map<int, std::vector<Param *>> links;
	
	std::optional<int> linked_controller; // ID of the linked MIDI controller of this param (if any)

	virtual void set_value(float value) = 0;
};

template<typename T>
struct Parameter : Param {
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
		for (auto const & option : options) assert(bounds.first <= option && option <= bounds.second);
	}

	void set_value(float value) override {
		auto [lower, upper] = bounds;

		constexpr bool is_float = std::is_same<T, float>();
		constexpr bool is_int   = std::is_same<T, int>();

		if (is_float) {
			parameter = util::lerp(lower, upper, value);
		} else if (is_int) {
			parameter = util::lerp(float(lower), float(upper), value);
		}
	}

	void render() {
		auto [lower, upper] = bounds;

		constexpr bool is_float = std::is_same<T, float>();
		constexpr bool is_int   = std::is_same<T, int>();

		char const * fmt = nullptr;
		
		float scroll_speed;

		// Render slider
		if constexpr (is_float) {
			ImGui::SliderFloat(name.c_str(), &parameter, lower, upper);

			fmt = "%.3f";

			scroll_speed = 0.01f;
		} else if constexpr (is_int) {
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

			auto any_clicked = false;

			for (auto const & option : options) {
				sprintf_s(label, fmt, option);

				if (ImGui::Button(label)) {
					parameter = option;

					any_clicked = true;
				}
			}

			if (options.size() > 0) ImGui::Separator();

			if (linked_controller.has_value()) {
				if (ImGui::Button("Unlink")) {
					auto & list = Param::links[linked_controller.value()];

					list.erase(std::find(list.begin(), list.end(), this));

					linked_controller = { };
				}

				ImGui::SameLine();
				ImGui::Text("0x%02x", linked_controller.value_or(0));
			} else {
				if (ImGui::Button("Link")) {
					Param::param_waiting_to_link = this;
				}

				if (Param::param_waiting_to_link == this) {
					ImGui::SameLine();
					ImGui::Text("Waiting for MIDI event...");
				}
			}

			ImGui::Separator();

			if (ImGui::Button("Copy")) {
				memcpy(Param::clipboard, &parameter, 4);

				any_clicked = true;
			}

			if (ImGui::Button("Paste")) {
				memcpy(&parameter, Param::clipboard, 4);
				parameter = util::clamp(parameter, lower, upper);

				any_clicked = true;
			}

			if (any_clicked) ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		} else if (Param::param_waiting_to_link == this) {
			Param::param_waiting_to_link = nullptr;
		}
	}

	operator T() const { return parameter; }
};
