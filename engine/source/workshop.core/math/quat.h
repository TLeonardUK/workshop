// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/vector4.h"

namespace ws {

template <typename element_type>
class base_quat
{
public:
	using vector3_type = base_vector3<element_type>;
	using vector4_type = base_vector4<element_type>;

	element_type x, y, z, w;

	static const base_quat<element_type> identity;

	base_quat() = default;
	base_quat(const base_quat& other) = default;
	base_quat(element_type _x, element_type _y, element_type _z, element_type _w);

	base_quat& operator=(const base_quat& other) = default;
	base_quat& operator+=(const base_quat& other);
	base_quat& operator-=(const base_quat& other);
	base_quat& operator*=(const base_quat& other);
	base_quat& operator*=(element_type scalar);
	base_quat& operator/=(const base_quat& other);
	base_quat& operator/=(element_type scalar);

	element_type length() const;
	base_quat<element_type> normalize() const;
	base_quat<element_type> conjugate() const;
	base_quat<element_type> inverse() const;

	static element_type dot(const base_quat<element_type>& a, const base_quat<element_type>& b);
	static base_quat<element_type> angle_axis(element_type angleRadians, const vector3_type& axis);
	static base_quat<element_type> euler(const vector3_type& angles);
};

typedef base_quat<float> quat;
typedef base_quat<double> quatd;

template <typename element_type>
inline const base_quat<element_type> base_quat<element_type>::identity = base_quat<element_type>(static_cast<element_type>(0), static_cast<element_type>(0), static_cast<element_type>(0), static_cast<element_type>(1));

template <typename element_type>
inline base_quat<element_type>::base_quat(element_type _x, element_type _y, element_type _z, element_type _w)
	: x(_x)
	, y(_y)
	, z(_z)
	, w(_w)
{
}

template <typename element_type>
inline base_quat<element_type>& base_quat<element_type>::operator+=(const base_quat& other)
{
	x += other.x;
	y += other.y;
	z += other.z;
	w += other.w;
	return *this;
}

template <typename element_type>
inline base_quat<element_type>& base_quat<element_type>::operator-=(const base_quat& other)
{
	x -= other.x;
	y -= other.y;
	z -= other.z;
	w -= other.w;
	return *this;
}

template <typename element_type>
inline base_quat<element_type>& base_quat<element_type>::operator*=(const base_quat& other)
{
	base_quat a(*this);
	base_quat b(other);

	w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
	x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
	y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
	z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;
	return *this;
}

template <typename element_type>
inline base_quat<element_type>& base_quat<element_type>::operator*=(element_type scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	w *= scalar;
	return *this;
}

template <typename element_type>
inline base_quat<element_type>& base_quat<element_type>::operator/=(const base_quat& other)
{
	x /= other.x;
	y /= other.y;
	z /= other.z;
	w /= other.w;
	return *this;
}

template <typename element_type>
inline base_quat<element_type>& base_quat<element_type>::operator/=(element_type scalar)
{
	x /= scalar;
	y /= scalar;
	z /= scalar;
	w /= scalar;
	return *this;
}

template <typename element_type>
inline element_type base_quat<element_type>::length() const
{
	return sqrt(dot(*this, *this));
}

template <typename element_type>
inline base_quat<element_type> base_quat<element_type>::normalize() const
{
	element_type len = length();
	if (len <= element_type(0))
	{
		return base_quat<element_type>(element_type(0), element_type(0), element_type(0), element_type(1));
	}
	element_type one_over_len = element_type(1) / len;
	return base_quat<element_type>(x * one_over_len, y * one_over_len, z * one_over_len, w * one_over_len);
}

template <typename element_type>
inline base_quat<element_type> base_quat<element_type>::conjugate() const
{
	return base_quat<element_type>(-x, -y, -z, w);
}

template <typename element_type>
inline base_quat<element_type> base_quat<element_type>::inverse() const
{
	return conjugate() / dot(*this, *this);
}

template <typename element_type>
inline static element_type base_quat<element_type>::dot(const base_quat<element_type>&a, const base_quat<element_type>&b)
{
	base_vector4<element_type> tmp(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
	return (tmp.x + tmp.y) + (tmp.z + tmp.w);
}

template <typename element_type>
inline static base_quat<element_type> base_quat<element_type>::angle_axis(element_type angle_radians, const vector3_type& axis)
{
	base_quat<element_type> result;

	element_type s = static_cast<element_type>(sin(angle_radians * static_cast<element_type>(0.5)));

	result.w = static_cast<element_type>(cos(angle_radians * static_cast<element_type>(0.5)));
	result.x = axis.x * s;
	result.y = axis.y * s;
	result.z = axis.z * s;

	return result;
}

template <typename element_type>
inline static base_quat<element_type> base_quat<element_type>::euler(const vector3_type& angles)
{
	base_quat<element_type> result;

	vector3_type c(cos(angles.x * element_type(0.5)), cos(angles.y * element_type(0.5)), cos(angles.z * element_type(0.5)));
	vector3_type s(sin(angles.x * element_type(0.5)), sin(angles.y * element_type(0.5)), sin(angles.z * element_type(0.5)));

	result.w = c.x * c.y * c.z + s.x * s.y * s.z;
	result.x = s.x * c.y * c.z - c.x * s.y * s.z;
	result.y = c.x * s.y * c.z + s.x * c.y * s.z;
	result.z = c.x * c.y * s.z - s.x * s.y * c.z;

	return result;
}

template <typename element_type>
inline base_quat<element_type> operator+(const base_quat<element_type>& first)
{
	return first;
}

template <typename element_type>
inline base_quat<element_type> operator-(const base_quat<element_type>& first)
{
	return base_quat<element_type>(-first.x, -first.y, -first.z, -first.w);
}

template <typename element_type>
inline base_quat<element_type> operator+(const base_quat<element_type>& first, const base_quat<element_type>& second)
{
	return base_quat<element_type>(first) += second;
}

template <typename element_type>
inline base_quat<element_type> operator*(const base_quat<element_type>& first, float second)
{
	return base_quat<element_type>(first) *= second;
}

template <typename element_type>
inline base_quat<element_type> operator*(const base_quat<element_type>& first, const base_quat<element_type>& second)
{
	return base_quat<element_type>(first) *= second;
}

template <typename element_type>
inline base_quat<element_type>::vector3_type operator*(const typename base_quat<element_type>::vector3_type& vec, const base_quat<element_type>& quat)
{
	return quat.Inverse() * vec;
}

template <typename element_type>
inline base_quat<element_type>::vector3_type operator*(const base_quat<element_type>& quat, const typename base_quat<element_type>::vector3_type& vec)
{
	typename base_quat<element_type>::vector3_type quatVector(quat.x, quat.y, quat.z);
	typename base_quat<element_type>::vector3_type uv(typename base_quat<element_type>::vector3_type::cross(quatVector, vec));
	typename base_quat<element_type>::vector3_type uuv(typename base_quat<element_type>::vector3_type::cross(quatVector, uv));

	return vec + ((uv * quat.w) + uuv) * static_cast<element_type>(2);
}

template <typename element_type>
inline base_quat<element_type>::vector4_type operator*(const typename base_quat<element_type>::vector4_type& vec, const base_quat<element_type>& quat)
{
	return quat.Inverse() * vec;
}

template <typename element_type>
inline base_quat<element_type>::vector4_type  operator*(const base_quat<element_type>& quat, const typename base_quat<element_type>::vector4_type& vec)
{
	typename base_quat<element_type>::vector3_type quatVector(vec.x, vec.y, vec.z);
	return typename base_quat<element_type>::vector4_type(quat * quatVector, 1.0f);
}

template <typename element_type>
inline base_quat<element_type> operator/(const base_quat<element_type>& first, float second)
{
	return base_quat<element_type>(first) /= second;
}

}; // namespace ws