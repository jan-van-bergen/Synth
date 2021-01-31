#pragma once
#include <cmath>
#include <array>

#include <SDL2/SDL_scancode.h>

inline constexpr auto     PI = 3.14159265359f;
inline constexpr auto TWO_PI = 6.28318530718f;

namespace util {
	template<typename T>
	inline T round_up(T value, T target) {
		auto remainder = value % target;

		if (remainder == 0) {
			return value;
		} else {
			return value + target - remainder;
		}
	}

	template<typename T>
	inline T lerp(T a, T b, float t) {
		return a + (b - a) * t;
	}

	template<typename T>
	inline T log_interpolate(T a, T b, float t) {
		return std::pow(a, 1.0f - t) * std::pow(b, t);
	}

	inline float clamp(float value, float min = 0.0f, float max = 1.0f) {
		if (value < min) return min;
		if (value > max) return max;

		return value;
	}
	
	template<typename T, int N>
	constexpr int array_element_count(const T (& array)[N]) {
		return N;
	}

	template<typename T, int N, typename Function>
	constexpr std::array<T, N> generate_lookup_table(Function function) {
		std::array<T, N> lut;

		for (int i = 0; i < N; i++) lut[i] = function(i);

		return lut;
	}

	int round(float f);

	int scancode_to_note(SDL_Scancode scancode);
	
	float note_freq(int note);
}
