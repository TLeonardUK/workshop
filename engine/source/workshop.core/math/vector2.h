// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/math.h"

namespace ws {

template <typename element_type>
class base_vector2
{
public:
	element_type x, y;

	static const base_vector2<element_type> zero;
	static const base_vector2<element_type> one;

	base_vector2() = default;
	base_vector2(const base_vector2& other) = default;
	base_vector2(element_type in_x, element_type in_y);

	base_vector2& operator=(const base_vector2& vector) = default;
	base_vector2& operator+=(element_type scalar);
	base_vector2& operator+=(const base_vector2& vector);
	base_vector2& operator-=(element_type scalar);
	base_vector2& operator-=(const base_vector2& vector);
	base_vector2& operator*=(element_type scalar);
	base_vector2& operator*=(const base_vector2& vector);
	base_vector2& operator/=(element_type scalar);
	base_vector2& operator/=(const base_vector2& vector);

	element_type length() const;
	element_type length_squared() const;
	base_vector2 round() const;
	base_vector2 abs() const;
	base_vector2 normalize() const;

	static float dot(const base_vector2& a, const base_vector2& b);
	static base_vector2 min(const base_vector2& a, const base_vector2& b);
	static base_vector2 max(const base_vector2& a, const base_vector2& b);
};

using vector2 = base_vector2<float>;
using vector2b = base_vector2<bool>;
using vector2i = base_vector2<int>;
using vector2u = base_vector2<unsigned int>;
using vector2d = base_vector2<double>;

template <typename element_type>
inline const base_vector2<element_type> base_vector2<element_type>::zero = base_vector2<element_type>(static_cast<element_type>(0), static_cast<element_type>(0));

template <typename element_type>
inline const base_vector2<element_type> base_vector2<element_type>::one = base_vector2<element_type>(static_cast<element_type>(1), static_cast<element_type>(1));

template <typename element_type>
inline base_vector2<element_type>::base_vector2(element_type in_x, element_type in_y)
	: x(in_x)
	, y(in_y)
{
}

template <typename element_type>
inline base_vector2<element_type>& base_vector2<element_type>::operator+=(element_type scalar)
{
	x += scalar;
	y += scalar;
	return *this;
}

template <typename element_type>
inline base_vector2<element_type>& base_vector2<element_type>::operator+=(const base_vector2& vector)
{
	x += vector.x;
	y += vector.y;
	return *this;
}

template <typename element_type>
inline base_vector2<element_type>& base_vector2<element_type>::operator-=(element_type scalar)
{
	x -= scalar;
	y -= scalar;
}

template <typename element_type>
inline base_vector2<element_type>& base_vector2<element_type>::operator-=(const base_vector2& vector)
{
	x -= vector.x;
	y -= vector.y;
	return *this;
}

template <typename element_type>
inline base_vector2<element_type>& base_vector2<element_type>::operator*=(element_type scalar)
{
	x *= scalar;
	y *= scalar;
}

template <typename element_type>
inline base_vector2<element_type>& base_vector2<element_type>::operator*=(const base_vector2<element_type>& vector)
{
	x *= vector.x;
	y *= vector.y;
	return *this;
}

template <typename element_type>
inline base_vector2<element_type>& base_vector2<element_type>::operator/=(element_type scalar)
{
	float inverse = 1.0f / scalar;
	x *= inverse;
	y *= inverse;
}

template <typename element_type>
inline base_vector2<element_type>& base_vector2<element_type>::operator/=(const base_vector2<element_type>& vector)
{
	x /= vector.x;
	y /= vector.y;
	return *this;
}

template <typename element_type>
inline element_type base_vector2<element_type>::length() const
{
	return sqrt(length_squared());
}

template <typename element_type>
inline element_type base_vector2<element_type>::length_squared() const
{
	return Dot(*this, *this);
}

template <typename element_type>
inline base_vector2<element_type> base_vector2<element_type>::normalize() const
{
	element_type length = length();
	if (length == 0.0f)
	{
		return *this;
	}
	else
	{
		element_type inveseLength = element_type(1.0) / length;
		return base_vector2<element_type>(x * inveseLength, y * inveseLength);
	}
}

template <typename element_type>
inline static float base_vector2<element_type>::dot(const base_vector2 & a, const base_vector2 & b)
{
	base_vector2<element_type> tmp(a * b);
	return tmp.x + tmp.y;
}

template <typename element_type>
inline base_vector2<element_type> base_vector2<element_type>::round() const
{
	return base_vector2(
		math::round(x),
		math::round(y)
	);
}

template <typename element_type>
inline base_vector2<element_type> base_vector2<element_type>::abs() const
{
	return base_vector2(
		math::abs(x),
		math::abs(y)
	);
}

template <typename element_type>
inline static base_vector2<element_type> base_vector2<element_type>::min(const base_vector2& a, const base_vector2& b)
{
	return base_vector2(
		a.x < b.x ? a.x : b.x,
		a.y < b.y ? a.y : b.y
	);
}

template <typename element_type>
inline static base_vector2<element_type> base_vector2<element_type>::max(const base_vector2& a, const base_vector2& b)
{
	return base_vector2(
		a.x > b.x ? a.x : b.x,
		a.y > b.y ? a.y : b.y
	);
}

template <typename element_type>
inline base_vector2<element_type> operator+(const base_vector2<element_type>& first)
{
	return base_vector2<element_type>(+first.x, +first.y);
}

template <typename element_type>
inline base_vector2<element_type> operator-(const base_vector2<element_type>& first)
{
	return base_vector2<element_type>(-first.x, -first.y);
}

template <typename element_type>
inline base_vector2<element_type> operator+(const base_vector2<element_type>& first, float second)
{
	return base_vector2<element_type>(first.x + second, first.y + second);
}

template <typename element_type>
inline base_vector2<element_type> operator+(const base_vector2<element_type>& first, const base_vector2<element_type>& second)
{
	return base_vector2<element_type>(first.x + second.x, first.y + second.y);
}

template <typename element_type>
inline base_vector2<element_type> operator-(const base_vector2<element_type>& first, float second)
{
	return base_vector2<element_type>(first.x - second, first.y - second);
}

template <typename element_type>
inline base_vector2<element_type> operator-(const base_vector2<element_type>& first, const base_vector2<element_type>& second)
{
	return base_vector2<element_type>(first.x - second.x, first.y - second.y);
}

template <typename element_type>
inline base_vector2<element_type> operator*(const base_vector2<element_type>& first, float second)
{
	return base_vector2<element_type>(first.x * second, first.y * second);
}

template <typename element_type>
inline base_vector2<element_type> operator*(const base_vector2<element_type>& first, const base_vector2<element_type>& second)
{
	return base_vector2<element_type>(first.x * second.x, first.y * second.y);
}

template <typename element_type>
inline base_vector2<element_type> operator/(const base_vector2<element_type>& first, float second)
{
	float inverse = 1.0f / second;
	return base_vector2<element_type>(first.x * inverse, first.y * inverse);
}

template <typename element_type>
inline base_vector2<element_type> operator/(const base_vector2<element_type>& first, const base_vector2<element_type>& second)
{
	return base_vector2<element_type>(first.x / second.x, first.y / second.y);
}

template <typename element_type>
inline bool operator==(const base_vector2<element_type>& first, const base_vector2<element_type>& second)
{
	return
		first.x == second.x &&
		first.y == second.y;
}

template <typename element_type>
inline bool operator!=(const base_vector2<element_type>& first, const base_vector2<element_type>& second)
{
	return !(first == second);
}

}; // namespace ws