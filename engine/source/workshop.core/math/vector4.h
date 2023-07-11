// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/math.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/filesystem/stream.h"

namespace ws {

template <typename element_type>
class base_vector4
{
public:
	element_type x, y, z, w;

	static const base_vector4<element_type> zero;
	static const base_vector4<element_type> one;

	base_vector4() = default;
	base_vector4(const base_vector4& other) = default;
	base_vector4(base_vector3<element_type> v, element_type _w);
	base_vector4(element_type _x, element_type _y, element_type _z, element_type _w);

	base_vector4& operator=(const base_vector4& vector) = default;
	base_vector4& operator+=(element_type scalar);
	base_vector4& operator+=(const base_vector4& vector);
	base_vector4& operator-=(element_type scalar);
	base_vector4& operator-=(const base_vector4& vector);
	base_vector4& operator*=(element_type scalar);
	base_vector4& operator*=(const base_vector4& vector);
	base_vector4& operator/=(element_type scalar);
	base_vector4& operator/=(const base_vector4& vector);

	element_type length() const;
	element_type length_squared() const;
	base_vector4 round() const;
	base_vector4 abs() const;
	base_vector4 normalize() const;

	static float dot(const base_vector4& a, const base_vector4& b);
	static base_vector4 min(const base_vector4& a, const base_vector4& b);
	static base_vector4 max(const base_vector4& a, const base_vector4& b);
};

using vector4 = base_vector4<float>;
using vector4b = base_vector4<bool>;
using vector4i = base_vector4<int>;
using vector4u = base_vector4<unsigned int>;
using vector4d = base_vector4<double>;

template <typename element_type>
inline const base_vector4<element_type> base_vector4<element_type>::zero = base_vector3<element_type>(static_cast<element_type>(0), static_cast<element_type>(0), static_cast<element_type>(0), static_cast<element_type>(0));

template <typename element_type>
inline const base_vector4<element_type> base_vector4<element_type>::one = base_vector3<element_type>(static_cast<element_type>(1), static_cast<element_type>(1), static_cast<element_type>(1), static_cast<element_type>(1));

template <typename element_type>
inline base_vector4<element_type>::base_vector4(base_vector3<element_type> v, element_type _w)
	: w(_w)
{
	x = v.x;
	y = v.y;
	z = v.z;
}

template <typename element_type>
inline base_vector4<element_type>::base_vector4(element_type _x, element_type _y, element_type _z, element_type _w)
	: x(_x)
	, y(_y)
	, z(_z)
	, w(_w)
{
}

template <typename element_type>
inline base_vector4<element_type>& base_vector4<element_type>::operator+=(element_type scalar)
{
	x += scalar;
	y += scalar;
	z += scalar;
	w += scalar;
	return *this;
}

template <typename element_type>
inline base_vector4<element_type>& base_vector4<element_type>::operator+=(const base_vector4& vector)
{
	x += vector.x;
	y += vector.y;
	z += vector.z;
	w += vector.w;
	return *this;
}

template <typename element_type>
inline base_vector4<element_type>& base_vector4<element_type>::operator-=(element_type scalar)
{
	x -= scalar;
	y -= scalar;
	z -= scalar;
	w -= scalar;
	return *this;
}

template <typename element_type>
inline base_vector4<element_type>& base_vector4<element_type>::operator-=(const base_vector4& vector)
{
	x -= vector.x;
	y -= vector.y;
	z -= vector.z;
	w -= vector.w;
	return *this;
}

template <typename element_type>
inline base_vector4<element_type>& base_vector4<element_type>::operator*=(element_type scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	w *= scalar;
	return *this;
}

template <typename element_type>
inline base_vector4<element_type>& base_vector4<element_type>::operator*=(const base_vector4& vector)
{
	x *= vector.x;
	y *= vector.y;
	z *= vector.z;
	w *= vector.w;
	return *this;
}

template <typename element_type>
inline base_vector4<element_type>& base_vector4<element_type>::operator/=(element_type scalar)
{
	float inverse = 1.0f / scalar;
	x *= inverse;
	y *= inverse;
	z *= inverse;
	w *= inverse;
	return *this;
}

template <typename element_type>
inline base_vector4<element_type>& base_vector4<element_type>::operator/=(const base_vector4& vector)
{
	x /= vector.x;
	y /= vector.y;
	z /= vector.z;
	w /= vector.w;
	return *this;
}

template <typename element_type>
inline element_type base_vector4<element_type>::length() const
{
	return sqrt(length_squared());
}

template <typename element_type>
inline element_type base_vector4<element_type>::length_squared() const
{
	return dot(*this, *this);
}

template <typename element_type>
inline base_vector4<element_type> base_vector4<element_type>::round() const
{
	return base_vector4(
		math::round(x),
		math::round(y),
		math::round(z),
		math::round(w)
	);
}

template <typename element_type>
inline base_vector4<element_type> base_vector4<element_type>::abs() const
{
	return base_vector4(
		math::abs(x),
		math::abs(y),
		math::abs(z),
		math::abs(w)
	);
}

template <typename element_type>
inline static base_vector4<element_type> base_vector4<element_type>::min(const base_vector4& a, const base_vector4& b)
{
	return base_vector4(
		a.x < b.x ? a.x : b.x,
		a.y < b.y ? a.y : b.y,
		a.z < b.z ? a.z : b.z,
		a.w < b.w ? a.w : b.w
	);
}

template <typename element_type>
inline static base_vector4<element_type> base_vector4<element_type>::max(const base_vector4& a, const base_vector4& b)
{
	return base_vector4(
		a.x > b.x ? a.x : b.x,
		a.y > b.y ? a.y : b.y,
		a.z > b.z ? a.z : b.z,
		a.w < b.w ? a.w : b.w
	);
}

template <typename element_type>
inline base_vector4<element_type> base_vector4<element_type>::normalize() const
{
	element_type len = length();
	if (len == 0.0f)
	{
		return *this;
	}
	else
	{
		element_type inveseLength = element_type(1.0) / len;
		return base_vector4<element_type>(x * inveseLength, y * inveseLength, z * inveseLength, w * inveseLength);
	}
}

template <typename element_type>
inline float base_vector4<element_type>::dot(const base_vector4 & a, const base_vector4 & b)
{
	base_vector4 tmp(a * b);
	return (tmp.x + tmp.y) + (tmp.z + tmp.w);
}

template <typename element_type>
inline base_vector4<element_type> operator+(const base_vector4<element_type>& first)
{
	return base_vector4<element_type>(+first.x, +first.y, +first.z, +first.w);
}

template <typename element_type>
inline base_vector4<element_type> operator-(const base_vector4<element_type>& first)
{
	return base_vector4<element_type>(-first.x, -first.y, -first.z, -first.w);
}

template <typename element_type>
inline base_vector4<element_type> operator+(const base_vector4<element_type>& first, float second)
{
	return base_vector4<element_type>(first.x + second, first.y + second, first.z + second, first.w + second);
}

template <typename element_type>
inline base_vector4<element_type> operator+(const base_vector4<element_type>& first, const base_vector4<element_type>& second)
{
	return base_vector4<element_type>(first.x + second.x, first.y + second.y, first.z + second.z, first.w + second.w);
}

template <typename element_type>
inline base_vector4<element_type> operator-(const base_vector4<element_type>& first, float second)
{
	return base_vector4<element_type>(first.x - second, first.y - second, first.z - second, first.w - second);
}

template <typename element_type>
inline base_vector4<element_type> operator-(const base_vector4<element_type>& first, const base_vector4<element_type>& second)
{
	return base_vector4<element_type>(first.x - second.x, first.y - second.y, first.z - second.z, first.w - second.w);
}

template <typename element_type>
inline base_vector4<element_type> operator*(const base_vector4<element_type>& first, float second)
{
	return base_vector4<element_type>(first.x * second, first.y * second, first.z * second, first.w * second);
}

template <typename element_type>
inline base_vector4<element_type> operator*(const base_vector4<element_type>& first, const base_vector4<element_type>& second)
{
	return base_vector4<element_type>(first.x * second.x, first.y * second.y, first.z * second.z, first.w * second.w);
}

template <typename element_type>
inline base_vector4<element_type> operator/(const base_vector4<element_type>& first, float second)
{
	float inverse = 1.0f / second;
	return base_vector4<element_type>(first.x * inverse, first.y * inverse, first.z * inverse, first.w * inverse);
}

template <typename element_type>
inline base_vector4<element_type> operator/(const base_vector4<element_type>& first, const base_vector4<element_type>& second)
{
	return base_vector4<element_type>(first.x / second.x, first.y / second.y, first.z / second.z, first.w / second.w);
}

template <typename element_type>
inline bool operator==(const base_vector4<element_type>& first, const base_vector4<element_type>& second)
{
	return
		first.x == second.x &&
		first.y == second.y &&
		first.z == second.z &&
		first.w == second.w;
}

template <typename element_type>
inline bool operator!=(const base_vector4<element_type>& first, const base_vector4<element_type>& second)
{
	return !(first == second);
}

template<>
inline void stream_serialize(stream& out, vector4& v)
{
	stream_serialize(out, v.x);
	stream_serialize(out, v.y);
	stream_serialize(out, v.z);
	stream_serialize(out, v.w);
}

}; // namespace ws