#pragma once
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include <ImGui/imgui.h>

#include "util.h"

#include "knob.h"

#include "json.h"

struct Component;

struct Param {
	inline static char clipboard[4] = { };

	inline static Param * param_waiting_to_link = nullptr;
	inline static std::unordered_map<int, std::vector<Param *>> links;
	
	char const * serialization_name;

	enum struct Curve { LINEAR, LOGARITHMIC } curve = Curve::LINEAR;

	std::optional<int> linked_controller; // ID of the linked MIDI controller of this param (if any)

	Param(Component * component, char const * serialization_name);

	virtual void set_value(float value) = 0;

	virtual void   serialize(json::Writer & writer) const = 0;
	virtual void deserialize(json::Object const & object) = 0;
};

template<typename T>
struct Parameter : Param {
	inline static constexpr auto IS_FLOAT = std::is_same<T, float>();
	inline static constexpr auto IS_INT   = std::is_same<T, int>();

	static_assert(IS_FLOAT || IS_INT);

	std::string name_short;
	std::string name_full;

	T parameter;
	T default_value;

	std::pair<T, T> bounds;

	std::vector<T> options; // Options available in context menu

	Parameter(Component * component, char const * serialization_name, std::string name_short, std::string name_full, T default_value, std::pair<T, T> bounds, std::vector<T> && options = { }, Curve curve = Curve::LINEAR) : 
		Param(component, serialization_name),
		name_short(name_short), 
		name_full(name_full), 
		parameter(default_value), 
		default_value(default_value),
		bounds(bounds), 
		options(options)
	{ 
		this->curve = curve;

		for (auto const & option : options) assert(bounds.first <= option && option <= bounds.second);
	}

	void set_value(float value) override {
		auto [lower, upper] = bounds;

		if (curve == Param::Curve::LINEAR) {
			parameter = util::lerp(float(lower), float(upper), value);
		} else {
			parameter = util::log_interpolate(float(lower), float(upper), value);
		}
	}

	void serialize(json::Writer & writer) const {
		writer.write(serialization_name, parameter);
	}

	void deserialize(json::Object const & object) {
		if constexpr (IS_FLOAT) {
			parameter = object.find_float(serialization_name, default_value);
		} else {
			parameter = object.find_int(serialization_name, default_value);
		}
	}

	bool render(util::Formatter<T> formatter = nullptr) {
		auto [lower, upper] = bounds;

		auto log_scale = curve == Param::Curve::LOGARITHMIC;

		char fmt[128] = { };
		if (formatter) {
			formatter(parameter, fmt, sizeof(fmt));
		}

		bool value_changed;

		// Render slider
		if constexpr (IS_FLOAT) {
			value_changed = ImGui::Knob(serialization_name, name_short.c_str(), name_full.c_str(), &parameter, lower, upper, log_scale, formatter);

			if (!formatter) strcpy_s(fmt, "%.2f");
		} else if constexpr (IS_INT) {
			value_changed = ImGui::Knob(serialization_name, name_short.c_str(), name_full.c_str(), &parameter, lower, upper, log_scale, formatter);

			if (!formatter) strcpy_s(fmt, "%i");
		} else {
			abort();
		}

		// Render default options context menu
		if (ImGui::BeginPopupContextItem()) {
			char label[128] = { };
			
			auto should_close_popup = false;

			for (auto const & option : options) {	
				if (formatter) {
					formatter(option, fmt, sizeof(fmt));
				}
				sprintf_s(label, fmt, option);

				if (ImGui::Button(label)) {
					parameter = option;

					should_close_popup = true;
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

				should_close_popup = true;
			}

			if (ImGui::Button("Paste")) {
				memcpy(&parameter, Param::clipboard, 4);
				parameter = util::clamp(parameter, lower, upper);

				should_close_popup = true;
			}

			if (should_close_popup) ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		} else if (Param::param_waiting_to_link == this) {
			Param::param_waiting_to_link = nullptr;
		}

		return value_changed;
	}

	operator T() const { return parameter; }

	void operator=(T const & value) {
		parameter = value;
	}
};
