#pragma once
#include <cmath>

inline constexpr auto SAMPLE_RATE     = 44100;
inline constexpr auto SAMPLE_RATE_INV = 1.0f / float(SAMPLE_RATE);

static constexpr int BLOCK_SIZE = 1024; // Samples are processed in blocks of this size


struct Sample {
	float left;
	float right;

	Sample()                 : left(0), right(0) { }
	Sample(float f)          : left(f), right(f) { }
	Sample(float l, float r) : left(l), right(r) { }

	static Sample floor(Sample const & s) { return { std::floor(s.left), std::floor(s.right) }; }

	static Sample lerp(Sample const & a, Sample const & b, Sample const & t);

	void operator+=(Sample const & other) { left += other.left; right += other.right; }
	void operator-=(Sample const & other) { left -= other.left; right -= other.right; }
	void operator*=(Sample const & other) { left *= other.left; right *= other.right; }
	void operator/=(Sample const & other) { left /= other.left; right /= other.right; }

	void operator+=(float f) { left += f; right += f; }
	void operator-=(float f) { left -= f; right -= f; }
	void operator*=(float f) { left *= f; right *= f; }
	void operator/=(float f) { left /= f; right /= f; }
};

inline Sample operator+(Sample const & a, Sample const & b) { return { a.left + b.left, a.right + b.right }; }
inline Sample operator-(Sample const & a, Sample const & b) { return { a.left - b.left, a.right - b.right }; }
inline Sample operator*(Sample const & a, Sample const & b) { return { a.left * b.left, a.right * b.right }; }
inline Sample operator/(Sample const & a, Sample const & b) { return { a.left / b.left, a.right / b.right }; }

inline Sample operator+(Sample const & s, float f) { return { s.left + f, s.right + f }; }
inline Sample operator-(Sample const & s, float f) { return { s.left - f, s.right - f }; }
inline Sample operator*(Sample const & s, float f) { return { s.left * f, s.right * f }; }
inline Sample operator/(Sample const & s, float f) { return { s.left / f, s.right / f }; }

inline Sample operator+(float f, Sample const & s) { return { s.left + f, s.right + f }; }
inline Sample operator-(float f, Sample const & s) { return { s.left - f, s.right - f }; }
inline Sample operator*(float f, Sample const & s) { return { s.left * f, s.right * f }; }
inline Sample operator/(float f, Sample const & s) { return { s.left / f, s.right / f }; }

inline Sample Sample::lerp(Sample const & a, Sample const & b, Sample const & t) { return a + (b - a) * t; }
