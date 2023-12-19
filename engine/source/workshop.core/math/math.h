// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <math.h>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace ws::math {

inline static constexpr float pi = 3.141592653f;
inline static constexpr float pi2 = pi * 2;
inline static constexpr float halfpi = pi * 0.5f;

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
	return static_cast<T>(a >= 0 ? 1 : -1);
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
	return static_cast<T>(::round(a));
}

template <typename T>
inline T saturate(T a)
{
	return static_cast<T>(std::clamp(a, 0.0f, 1.0f));
}

// Rounds up a value to a given multiple.
// eg. value=8  multiple=16 result=16
// eg. value=17 multiple=16 result=32 
template <typename T>
inline T round_up_multiple(T value, T multiple_of)
{
	if (multiple_of == 0 || multiple_of == 1)
	{
		return value;
	}
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

template <typename T>
inline T calculate_standard_deviation(auto begin_iter, auto end_iter)
{
    size_t count = std::distance(begin_iter, end_iter);
    T mean = std::accumulate(begin_iter, end_iter, (T)0.0) / count;
    
    auto variance = [&mean, &count](T accumulator, const T& value) {
        return accumulator + ((value - mean) * (value - mean));
    };

    T sum = std::accumulate(begin_iter, end_iter, (T)0.0, variance);
    sum /= count;

    return (T)sqrtf((float)sum);
}

template <typename T>
inline T calculate_mean(auto begin_iter, auto end_iter)
{
    size_t count = std::distance(begin_iter, end_iter);
    return std::accumulate(begin_iter, end_iter, (T)0.0) / count;
}

// To/from IEE-754 16-bit floating point values, as used by most image formats.
// Based on code from:
// https://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion

template <typename in_type, typename out_type>
out_type as_format(const in_type x) {
	return *(out_type*)&x;
}

inline uint16_t to_float16(float value)
{
	const uint32_t b = as_format<float, uint32_t>(value) + 0x00001000;						// round-to-nearest-even: add last bit after truncated mantissa
	const uint32_t e = (b & 0x7F800000) >> 23;												// exponent
	const uint32_t m = b & 0x007FFFFF;														// mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding

	return (b & 0x80000000) >> 16 | 
			(e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) | 
			((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) | 
			(e > 143) * 0x7FFF;																// sign : normalized : denormalized : saturate
}

inline float from_float16(uint16_t value)
{
	const uint32_t e = (value & 0x7C00) >> 10;												// exponent
	const uint32_t m = (value & 0x03FF) << 13;												// mantissa
	const uint32_t v = (as_format<float, uint32_t>((float)m)) >> 23;						// evil log2 bit hack to count leading zeros in denormalized format

	return as_format<uint32_t, float>((value & 0x8000) << 16 |
			(e != 0) * ((e + 112) << 23 | m) | 
			((e == 0) & (m != 0)) * ((v - 37) << 23 | 
			((m << (150 - v)) & 0x007FE000)));												// sign : normalized : denormalized
}

inline uint8_t float_to_byte(float value)
{
	value = (value + 1.0f) * 0.5f;
	return static_cast<uint8_t>(value * 255.0f);
}

inline float byte_to_float(uint8_t value)
{
	float ret = (value / 255.0f);
	ret = (ret * 2.0f) - 1.0f;
	return ret;
}

// Gets the size an atlas rectangle needs to be to fit the given number of sub rectangles.
// Sizes increate in powers of two.
inline size_t calculate_atlas_size(size_t sub_rect_size, size_t sub_rect_count)
{
    size_t atlas_size = 1;
    while (true)
    {
        size_t per_row = (atlas_size / sub_rect_size);
        size_t fit_count = per_row * per_row;
        if (fit_count >= sub_rect_count)
        {
            break;
        }
        atlas_size = atlas_size * 2;
    }

    return atlas_size;
}

}; // namespace ws::math