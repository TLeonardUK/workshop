// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <math.h>
#include <cmath>

namespace ws::math {

inline static constexpr float pi = 3.141592653f;

template <typename T>
inline T radians(T degrees)
{
	static constexpr float factor = pi / T(180);
	return degrees * factor;
}

template <typename T>
inline T degrees(T radians)
{
	static constexpr float factor = T(180) / pi;
	return radians * factor;
}

template <typename T>
inline T min(T a, T b)
{
	return a < b ? a : b;
}

template <typename T>
inline T max(T a, T b)
{
	return a > b ? a : b;
}

template <typename T>
inline T sign(T a)
{
	return (a >= 0 ? 1 : -1);
}

inline float sqrt(float in)
{
	return ::sqrtf(in);
}

inline float inv_sqrt(float in)
{
	return 1.0f / ::sqrtf(in);
}

template <typename T>
inline T ceil(T in)
{
	return ::ceil(in);
}

template <typename T>
inline T pow(T a, T b)
{
	return ::pow(a, b);
}

template <typename T>
inline T square(T in)
{
	return in * in;
}

template <typename T>
inline T lerp(T a, T b, T delta)
{
	return a + ((b - a) * delta);
}

template <typename T>
inline T mod(T a, T b)
{
	return std::remainder(a, b);
}

template <typename T>
inline T abs(T a)
{
	return static_cast<T>(::abs(a));
}

template <typename T>
inline T round(T a)
{
	return static_cast<T>(round(a));
}

// Rounds up a value to a given multiple.
// eg. value=8  multiple=16 result=16
// eg. value=17 multiple=16 result=32 
template <typename T>
inline T round_up_multiple(T value, T multiple_of)
{
	return static_cast<int>((value + (multiple_of - 1)) / multiple_of) * multiple_of;
}

template <typename T>
inline T next_power_of_two(T v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

template <typename T>
inline bool in_range(T value, T min, T max)
{
	return (value >= min && value < max);
}

template <typename T>
inline bool in_range_inclusive(T value, T min, T max)
{
	return (value >= min && value <= max);
}

}; // namespace ws::math