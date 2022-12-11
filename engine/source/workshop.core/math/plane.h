// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/aabb.h"
#include "workshop.core/math/math.h"

namespace ws {

class plane
{
public:
	enum class classification
	{
		behind,
		infront,
		intersecting
	};

public:
	float x, y, z, w;

	plane() = default;
	plane(float x_, float y_, float z_, float w_);
	plane(const vector3& normal, const vector3& origin);

	vector3 get_normal() const;
	float get_distance() const;

	classification classify(const vector3& point) const;

	plane normalize() const;
	plane flip() const;

	static vector3 intersect(const plane& plane1, const plane& plane2, const plane& plane3);

	static float dot(const plane& plane1, const plane& plane2);
	static float dot(const plane& plane, const vector3& point);

};

inline plane::plane(float x_, float y_, float z_, float w_)
	: x(x_)
	, y(y_)
	, z(z_)
	, w(w_)
{
}

inline plane::plane(const vector3& normal, const vector3& origin)
{
	x = normal.x;
	y = normal.y;
	z = normal.z;
	w = vector3::dot(normal, origin);
}

inline vector3 plane::get_normal() const
{
	return vector3(x, y, z);
}

inline float plane::get_distance() const
{
	return w;
}

inline plane::classification plane::classify(const vector3& point) const
{
	float d = dot(*this, point);
	if (d < 0.0f)
	{
		return classification::behind;
	}
	else if (d > 0.0f)
	{
		return classification::infront;
	}
	else
	{
		return classification::intersecting;
	}
}

inline plane plane::normalize() const
{
	float sum = x * x + y * y + z * z;
	float invSqrt = math::inv_sqrt(sum);

	return plane(x * invSqrt, y * invSqrt, z * invSqrt, w * invSqrt);
}

inline plane plane::flip() const
{
	return plane(-x, -y, -z, -w);
}

inline vector3 plane::intersect(const plane& plane1, const plane& plane2, const plane& plane3)
{
	// Faster method, but will not handle situations such as planes not all intersecting,
	// thus giving multiple line-segments intersections.

	vector3 p0 = plane1.get_normal();
	vector3 p1 = plane2.get_normal();
	vector3 p2 = plane3.get_normal();

	vector3 bxc = vector3::cross(p1, p2);
	vector3 cxa = vector3::cross(p2, p0);
	vector3 axb = vector3::cross(p0, p1);
	vector3 r = -plane1.w * bxc - plane2.w * cxa - plane3.w * axb;

	return r * (1 / vector3::dot(p0, bxc));

	/*
	Vector3 abDet = Vector3::Determinant(plane1Vec, plane2Vec);
	float determinant = Vector3::Dot(abDet, plane3Vec);
	if (Math::Squared(determinant) < Math::Squared(0.001f))
	{
		return Vector3::Zero;
	}
	else
	{
		Vector3 bcDet = Vector3::Determinant(plane2Vec, plane3Vec);
		Vector3 caDet = Vector3::Determinant(plane3Vec, plane1Vec);
		return (plane1.w * bcDet + plane2.w * caDet + plane3.w * abDet) / determinant;
	}*/
}

inline float plane::dot(const plane& plane1, const plane& plane2)
{
	return plane1.x * plane2.x + plane1.y * plane2.y + plane1.z * plane2.z + plane1.w * plane2.w;
}

inline float plane::dot(const plane& plane, const vector3& point)
{
	return plane.x * point.x + plane.y * point.y + plane.z * point.z - plane.w;
}

}; // namespace ws
