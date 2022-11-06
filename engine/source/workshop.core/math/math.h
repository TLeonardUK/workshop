// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <math.h>
#include <cmath>

namespace ws::math {

static const float Pi = 3.141592653f;

template <typename T>
inline T radians(T degrees)
{
	static const float factor = Pi / T(180);
	return degrees * factor;
}

template <typename T>
inline T degrees(T radians)
{
	static const float factor = T(180) / Pi;
	return radians * factor;
}
	
template <typename T>
inline T sign(T a)
{
	return (a >= 0 ? 1 : -1);
}

inline float inv_sqrt(float in)
{
	return 1.0f / sqrtf(in);
}

template <typename T>
inline T squared(T in)
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