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
	inline static float clipboard_value = { };

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

		char fmt[128] = { };
		if (formatter) {
			formatter(parameter, fmt, sizeof(fmt));
		} else {
			if constexpr (IS_FLOAT) {
				strcpy_s(fmt, "%.2f");
			} else {
				strcpy_s(fmt, "%i");
			}
		}

		auto value_changed = ImGui::Knob(serialization_name, name_short.c_str(), name_full.c_str(), &parameter, lower, upper, curve == Param::Curve::LOGARITHMIC, fmt);

		// Render default options context menu
		if (ImGui::BeginPopupContextItem()) {
			char label[128] = { };
			
			auto should_close_context_menu = false;

			for (auto const & option : options) {	
				if (formatter) {
					formatter(option, fmt, sizeof(fmt));
				}
				sprintf_s(label, fmt, option);

				if (ImGui::Button(label)) {
					parameter = option;

					should_close_context_menu = true;
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

			static char enter_text[128] = { };
			
			if (ImGui::Button("Enter Value")) {
				ImGui::OpenPopup("Enter");
				
				if constexpr (IS_FLOAT) { // NOTE: No custom formatting here
					sprintf_s(enter_text, "%f", parameter);
				} else {
					sprintf_s(enter_text, "%i", parameter);
				}
			}

			if (ImGui::BeginPopup("Enter")) {
				ImGui::SetKeyboardFocusHere(0);
				ImGui::InputText("Value", enter_text, sizeof(enter_text));

				if (ImGui::IsKeyPressed(SDL_SCANCODE_RETURN)) {
					if constexpr (IS_FLOAT) {
						parameter = std::atof(enter_text);
					} else {
						parameter = std::atoi(enter_text);
					}

					parameter = util::clamp(parameter, lower, upper);
				}

				if (ImGui::IsKeyPressed(SDL_SCANCODE_RETURN) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
					ImGui::CloseCurrentPopup();
					should_close_context_menu = true;
				}

				ImGui::EndPopup();
			}

			if (ImGui::Button("Copy")) {
				clipboard_value = parameter;

				should_close_context_menu = true;
			}

			if (ImGui::Button("Paste")) {
				if constexpr (IS_FLOAT) {
					parameter = clipboard_value;
				} else {
					parameter = util::round(clipboard_value);
				}

				parameter = util::clamp(parameter, lower, upper);

				should_close_context_menu = true;
			}

			if (should_close_context_menu) ImGui::CloseCurrentPopup();

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

    template<typename = std::enable_if_t<IS_FLOAT>()> static Parameter<float> make_attack (Component * component, char const * name = "attack")  { return Parameter<float>(component, name, "Att",  "Attack",  0.1f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 }); }
    template<typename = std::enable_if_t<IS_FLOAT>()> static Parameter<float> make_hold   (Component * component, char const * name = "hold")    { return Parameter<float>(component, name, "Hold", "Hold",    0.5f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 }); }
    template<typename = std::enable_if_t<IS_FLOAT>()> static Parameter<float> make_decay  (Component * component, char const * name = "decay")   { return Parameter<float>(component, name, "Dec",  "Decay",   1.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 }); }
    template<typename = std::enable_if_t<IS_FLOAT>()> static Parameter<float> make_sustain(Component * component, char const * name = "sustain") { return Parameter<float>(component, name, "Sus",  "Sustain", 0.5f, std::make_pair(0.0f, 1.0f)); }
    template<typename = std::enable_if_t<IS_FLOAT>()> static Parameter<float> make_release(Component * component, char const * name = "release") { return Parameter<float>(component, name, "Rel",  "Release", 0.0f, std::make_pair(0.0f, 16.0f), { 1, 2, 3, 4, 8, 16 }); }
};
