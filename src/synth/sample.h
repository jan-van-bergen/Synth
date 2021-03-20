#pragma once

inline constexpr auto SAMPLE_RATE     = 44100;
inline constexpr auto SAMPLE_RATE_INV = 1.0f / float(SAMPLE_RATE);

inline constexpr auto BLOCK_SIZE     = 512; // Samples are processed in blocks of this size
inline constexpr auto BLOCK_SIZE_INV = 1.0f / float(BLOCK_SIZE);

struct Sample {
	float left;
	float right;

	Sample()                 : left(0), right(0) { }
	Sample(float f)          : left(f), right(f) { }
	Sample(float l, float r) : left(l), right(r) { }

	template<typename Function>
	static Sample apply_function(Function func, Sample const & sample) { return { func(sample.left), func(sample.right) }; }

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

inline Sample operator+(Sample const & s) { return s; }
inline Sample operator-(Sample const & s) { return Sample(-s.left, -s.right); }

inline bool operator==(Sample const & a, Sample const & b) { return a.left == b.left && a.right == b.right; }
inline bool operator!=(Sample const & a, Sample const & b) { return a.left != b.left || a.right != b.right; }
