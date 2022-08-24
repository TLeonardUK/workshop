// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <math.h>

namespace ws::math {

static const float Pi = 3.141592653f;

template <typename T>
T radians(T degrees)
{
	static const float factor = Pi / T(180);
	return degrees * factor;
}

template <typename T>
T degrees(T radians)
{
	static const float factor = T(180) / Pi;
	return radians * factor;
}
	
template <typename T>
T sign(T a)
{
	return (a >= 0 ? 1 : -1);
}

inline float inv_sqrt(float in)
{
	return 1.0f / sqrtf(in);
}

template <typename T>
T squared(T in)
{
	return in * in;
}

template <typename T>
T lerp(T a, T b, T delta)
{
	return a + ((b - a) * delta);
}

template <typename T>
T mod(T a, T b)
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

}; // namespace ws::math