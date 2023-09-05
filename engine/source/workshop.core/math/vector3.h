// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/math.h"
#include "workshop.core/math/vector2.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/utils/yaml.h"

namespace ws {

template <typename element_type>
class base_vector3
{
public:
	element_type x, y, z;

	static const base_vector3<element_type> zero;
	static const base_vector3<element_type> one;
	static const base_vector3<element_type> forward;
	static const base_vector3<element_type> right;
	static const base_vector3<element_type> up;

	base_vector3() = default;
	base_vector3(const base_vector3& other) = default;
	base_vector3(base_vector2<element_type> v, element_type _z);
	base_vector3(element_type _x, element_type _y, element_type _z);

	base_vector3& operator=(const base_vector3& vector) = default;
	base_vector3& operator+=(element_type scalar);
	base_vector3& operator+=(const base_vector3& vector);
	base_vector3& operator-=(element_type scalar);
	base_vector3& operator-=(const base_vector3& vector);
	base_vector3& operator*=(element_type scalar);
	base_vector3& operator*=(const base_vector3& vector);
	base_vector3& operator/=(element_type scalar);
	base_vector3& operator/=(const base_vector3& vector);

	element_type length() const;
	element_type length_squared() const;
	base_vector3 round() const;
	base_vector3 abs() const;
	base_vector3 normalize() const;

	element_type max_component() const;
	element_type min_component() const;

	static base_vector3 determinant(const base_vector3& a, const base_vector3& b);
	static float dot(const base_vector3& a, const base_vector3& b);
	static base_vector3 cross(const base_vector3& a, const base_vector3& b);
	static base_vector3 min(const base_vector3& a, const base_vector3& b);
	static base_vector3 max(const base_vector3& a, const base_vector3& b);
};

using vector3  = base_vector3<float>;
using vector3b = base_vector3<bool>;
using vector3i = base_vector3<int>;
using vector3u = base_vector3<unsigned int>;
using vector3d = base_vector3<double>;

template <typename element_type>
inline const base_vector3<element_type> base_vector3<element_type>::zero = base_vector3<element_type>(static_cast<element_type>(0), static_cast<element_type>(0), static_cast<element_type>(0));

template <typename element_type>
inline const base_vector3<element_type> base_vector3<element_type>::one = base_vector3<element_type>(static_cast<element_type>(1), static_cast<element_type>(1), static_cast<element_type>(1));

template <typename element_type>
inline const base_vector3<element_type> base_vector3<element_type>::forward = base_vector3<element_type>(static_cast<element_type>(0), static_cast<element_type>(0), static_cast<element_type>(1));

template <typename element_type>
inline const base_vector3<element_type> base_vector3<element_type>::right = base_vector3<element_type>(static_cast<element_type>(1), static_cast<element_type>(0), static_cast<element_type>(0));

template <typename element_type>
inline const base_vector3<element_type> base_vector3<element_type>::up = base_vector3<element_type>(static_cast<element_type>(0), static_cast<element_type>(1), static_cast<element_type>(0));

template <typename element_type>
inline base_vector3<element_type>::base_vector3(base_vector2<element_type> v, element_type _z)
	: z(_z)
{
	x = v.x;
	y = v.y;
}

template <typename element_type>
inline base_vector3<element_type>::base_vector3(element_type _x, element_type _y, element_type _z)
	: x(_x)
	, y(_y)
	, z(_z)
{
}

template <typename element_type>
inline base_vector3<element_type>& base_vector3<element_type>::operator+=(element_type scalar)
{
	x += scalar;
	y += scalar;
	z += scalar;
	return *this;
}

template <typename element_type>
inline base_vector3<element_type>& base_vector3<element_type>::operator+=(const base_vector3 & vector)
{
	x += vector.x;
	y += vector.y;
	z += vector.z;
	return *this;
}

template <typename element_type>
inline base_vector3<element_type>& base_vector3<element_type>::operator-=(element_type scalar)
{
	x -= scalar;
	y -= scalar;
	z -= scalar;
	return *this;
}

template <typename element_type>
inline base_vector3<element_type>& base_vector3<element_type>::operator-=(const base_vector3 & vector)
{
	x -= vector.x;
	y -= vector.y;
	z -= vector.z;
	return *this;
}

template <typename element_type>
inline base_vector3<element_type>& base_vector3<element_type>::operator*=(element_type scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	return *this;
}

template <typename element_type>
inline base_vector3<element_type>& base_vector3<element_type>::operator*=(const base_vector3 & vector)
{
	x *= vector.x;
	y *= vector.y;
	z *= vector.z;
	return *this;
}

template <typename element_type>
inline base_vector3<element_type>& base_vector3<element_type>::operator/=(element_type scalar)
{
	float inverse = 1.0f / scalar;
	x *= inverse;
	y *= inverse;
	z *= inverse;
	return *this;
}
template <typename element_type>
inline base_vector3<element_type>& base_vector3<element_type>::operator/=(const base_vector3 & vector)
{
	x /= vector.x;
	y /= vector.y;
	z /= vector.z;
	return *this;
}

template <typename element_type>
inline element_type base_vector3<element_type>::length() const
{
	return sqrt(length_squared());
}

template <typename element_type>
inline element_type base_vector3<element_type>::length_squared() const
{
	return dot(*this, *this);
}

template <typename element_type>
inline element_type base_vector3<element_type>::max_component() const
{
	return std::max(x, std::max(y, z));
}

template <typename element_type>
inline element_type base_vector3<element_type>::min_component() const
{
	return std::min(x, std::min(y, z));
}

template <typename element_type>
inline base_vector3<element_type> base_vector3<element_type>::round() const
{
	return base_vector3(
		math::round(x),
		math::round(y),
		math::round(z)
	);
}

template <typename element_type>
inline base_vector3<element_type> base_vector3<element_type>::abs() const
{
	return base_vector3(
		math::abs(x), 
		math::abs(y), 
		math::abs(z)
	);
}

template <typename element_type>
inline base_vector3<element_type> base_vector3<element_type>::normalize() const
{
	element_type len = length();
	if (len == 0.0f)
	{
		return *this;
	}
	else
	{
		element_type inveseLength = element_type(1.0) / len;
		return base_vector3<element_type>(x * inveseLength, y * inveseLength, z * inveseLength);
	}
}

template <typename element_type>
inline base_vector3<element_type> base_vector3<element_type>::determinant(const base_vector3 & a, const base_vector3 & b)
{
	return base_vector3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

template <typename element_type>
inline float base_vector3<element_type>::dot(const base_vector3 & a, const base_vector3 & b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename element_type>
inline base_vector3<element_type> base_vector3<element_type>::cross(const base_vector3 & a, const base_vector3 & b)
{
	return base_vector3(
		a.y * b.z - b.y * a.z,
		a.z * b.x - b.z * a.x,
		a.x * b.y - b.x * a.y
	);
}

template <typename element_type>
inline base_vector3<element_type> base_vector3<element_type>::min(const base_vector3 & a, const base_vector3 & b)
{
	return base_vector3(
		a.x < b.x ? a.x : b.x,
		a.y < b.y ? a.y : b.y,
		a.z < b.z ? a.z : b.z
	);
}

template <typename element_type>
inline base_vector3<element_type> base_vector3<element_type>::max(const base_vector3 & a, const base_vector3 & b)
{
	return base_vector3(
		a.x > b.x ? a.x : b.x,
		a.y > b.y ? a.y : b.y,
		a.z > b.z ? a.z : b.z
	);
}

template <typename element_type>
inline base_vector3<element_type> operator+(const base_vector3<element_type>& first)
{
	return base_vector3<element_type>(+first.x, +first.y, +first.z);
}

template <typename element_type>
inline base_vector3<element_type> operator-(const base_vector3<element_type>& first)
{
	return base_vector3<element_type>(-first.x, -first.y, -first.z);
}

template <typename element_type>
inline base_vector3<element_type> operator+(const base_vector3<element_type>& first, float second)
{
	return base_vector3<element_type>(first.x + second, first.y + second, first.z + second);
}

template <typename element_type>
inline base_vector3<element_type> operator+(const base_vector3<element_type>& first, const base_vector3<element_type>& second)
{
	return base_vector3<element_type>(first.x + second.x, first.y + second.y, first.z + second.z);
}

template <typename element_type>
inline base_vector3<element_type> operator-(const base_vector3<element_type>& first, float second)
{
	return base_vector3<element_type>(first.x - second, first.y - second, first.z - second);
}

template <typename element_type>
inline base_vector3<element_type> operator-(const base_vector3<element_type>& first, const base_vector3<element_type>& second)
{
	return base_vector3<element_type>(first.x - second.x, first.y - second.y, first.z - second.z);
}

template <typename element_type>
inline base_vector3<element_type> operator*(const base_vector3<element_type>& first, float second)
{
	return base_vector3<element_type>(first.x * second, first.y * second, first.z * second);
}

template <typename element_type>
inline base_vector3<element_type> operator*(float second, const base_vector3<element_type>& first)
{
	return base_vector3<element_type>(first.x * second, first.y * second, first.z * second);
}

template <typename element_type>
inline base_vector3<element_type> operator*(const base_vector3<element_type>& first, const base_vector3<element_type>& second)
{
	return base_vector3<element_type>(first.x * second.x, first.y * second.y, first.z * second.z);
}

template <typename element_type>
inline base_vector3<element_type> operator/(const base_vector3<element_type>& first, float second)
{
	float inverse = 1.0f / second;
	return base_vector3<element_type>(first.x * inverse, first.y * inverse, first.z * inverse);
}

template <typename element_type>
inline base_vector3<element_type> operator/(const base_vector3<element_type>& first, const base_vector3<element_type>& second)
{
	return base_vector3<element_type>(first.x / second.x, first.y / second.y, first.z / second.z);
}

template <typename element_type>
inline bool operator==(const base_vector3<element_type>& first, const base_vector3<element_type>& second)
{
	return
		first.x == second.x &&
		first.y == second.y &&
		first.z == second.z;
}

template <typename element_type>
inline bool operator!=(const base_vector3<element_type>& first, const base_vector3<element_type>& second)
{
	return !(first == second);
}

template<>
inline void stream_serialize(stream& out, vector3& v)
{
	stream_serialize(out, v.x);
	stream_serialize(out, v.y);
	stream_serialize(out, v.z);
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, vector3& value)
{
	YAML::Node x = out["x"];
	YAML::Node y = out["y"];
	YAML::Node z = out["z"];

	yaml_serialize(x, is_loading, value.x);
	yaml_serialize(y, is_loading, value.y);
	yaml_serialize(z, is_loading, value.z);
}

}; // namespace ws