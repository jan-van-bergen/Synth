#pragma once
#include <cmath>
#include <cstring>

#include <array>
#include <vector>

#include <typeinfo>

#include <SDL2/SDL_scancode.h>

#include "sample.h"

inline constexpr auto     PI = 3.14159265359f;
inline constexpr auto TWO_PI = 6.28318530718f;

namespace util {
	template<typename T>
	inline constexpr T round_up(T value, T target) {
		auto remainder = value % target;

		if (remainder == 0) {
			return value;
		} else {
			return value + target - remainder;
		}
	}

	template<typename T, typename L = T>
	inline constexpr T lerp(T a, T b, L t) {
		return a + (b - a) * t;
	}

	template<typename T>
	inline T log_interpolate(T a, T b, float t) {
		return std::pow(a, 1.0f - t) * std::pow(b, t);
	}

	template<typename T>
	inline constexpr T clamp(T value, T min = static_cast<T>(0), T max = static_cast<T>(1)) {
		assert(min <= max);

		if (value < min) return min;
		if (value > max) return max;

		return value;
	}
	
	template<typename T>
	inline constexpr T wrap(T value, T max) {
		assert(value >=    -max);
		assert(value <= 2 * max);

		if (value < 0)    return value + max;
		if (value >= max) return value - max;

		return value;
	}

	template<typename T>
	inline T remap(T value, T old_min, T old_max, T new_min, T new_max) {
		assert(old_min <= value && value <= old_max);

		return new_min + (value - old_min) / (old_max - old_min) * (new_max - new_min);
	}

	template<typename T, int N>
	inline constexpr int array_element_count(const T (& array)[N]) {
		return N;
	}

	template<typename T, int N, typename Function>
	inline constexpr std::array<T, N> generate_lookup_table(Function function) {
		std::array<T, N> lut;

		for (int i = 0; i < N; i++) lut[i] = function(i);

		return lut;
	}

	inline constexpr int log2(int x) {
		auto log = 0;
		
		while (x > 1) {
			x /= 2;
			log++;
		}
		
		return log;
	}

	inline constexpr bool is_power_of_two(int x) {
		return x > 0 && (x & (x - 1)) == 0;
	}
	
	int round(float f);

	template<typename T>
	inline T sample_linear(T array[], int length, float x, bool wrap = true) {
		auto index_a = util::round(x - 0.5f);
		auto index_b = index_a + 1;

		if (wrap) {
			index_a = util::wrap(index_a, length);
			index_b = util::wrap(index_b, length);
		} else if (x <= 0.0f) {
			return array[0];
		} else if (x >= float(length) - 1.0f) {
			return array[length - 1];
		}
		
		auto weight = x - std::floor(x);

		return util::lerp(array[index_a], array[index_b], weight);
	}

	inline float linear_to_db(float x) { return 20.0f * std::log10(x); }
	inline float db_to_linear(float x) { return std::pow(10.0f, x / 20.0f); }

	template<typename T>
	inline constexpr char const * get_type_name() {
		auto name = typeid(T).name();

		if (memcmp(name, "struct ", 7) == 0) return name + 7;
		if (memcmp(name, "class ",  6) == 0) return name + 6;

		return name;
	}
	
	template<typename T>
	inline constexpr char const * get_type_name(T const & object) {
		auto name = typeid(object).name();

		if (memcmp(name, "struct ", 7) == 0) return name + 7;
		if (memcmp(name, "class ",  6) == 0) return name + 6;

		return name;
	}
	
	std::vector<Sample> load_wav(char const * filename);

	std::vector<char> read_file(char const * filename);

	int scancode_to_note(SDL_Scancode scancode);
	
	float note_freq(int note);

	void note_name(int note, char str[], int len);
}
