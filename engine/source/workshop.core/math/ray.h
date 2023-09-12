// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/triangle.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/aabb.h"
#include "workshop.core/math/math.h"

namespace ws {

class ray
{
public:
	vector3 start = vector3::zero;
	vector3 end = vector3::zero;

	vector3 direction = vector3::zero;
	float length = 0.0f;

	ray() = default;
	ray(const vector3& start, const vector3& end);

	bool intersects(const triangle& tri, vector3* intersect_point = nullptr) const;
	bool intersects(const aabb& bounds, vector3* intersect_point = nullptr) const;
};

inline ray::ray(const vector3& in_start, const vector3& in_end)
{
	start = in_start;
	end = in_end;

	// We calculate this up front as its rare that this isn't needed.
	vector3 diff = (end - start);
	length = diff.length();
	direction = diff.normalize();
}

inline bool ray::intersects(const triangle& tri, vector3* intersect_point) const
{
	// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm)

	constexpr float k_epsilon = 0.00001f;

	vector3 e1 = tri.b - tri.a;
	vector3 e2 = tri.c - tri.a;
	vector3 n = vector3::cross(e1, e2);

	float det = -vector3::dot(direction, n);
	if (det < k_epsilon)
	{
		return false;
	}

	float invdet = 1.0f / det;

	vector3 ao = start - tri.a;
	vector3 dao = vector3::cross(ao, direction);

	float u = vector3::dot(e2, dao) * invdet;
	if (u < 0.0f || u > 1.0f)
	{
		return false;
	}

	float v = -vector3::dot(e1, dao) * invdet;
	if (v < 0.0f || u + v > 1.0f)
	{
		return false;
	}

	float t = vector3::dot(ao, n) * invdet;
	if (t >= k_epsilon)
	{
		if (intersect_point)
		{
			*intersect_point = start + t * direction;
		}
		return true;
	}
	else
	{
		return false;
	}
}

inline bool ray::intersects(const aabb& bounds, vector3* intersect_point) const
{
	// https://tavianator.com/2011/ray_box.html

	float t1 = (bounds.min.x - start.x) / direction.x;
	float t2 = (bounds.max.x - start.x) / direction.x;
	float t3 = (bounds.min.y - start.y) / direction.y;
	float t4 = (bounds.max.y - start.y) / direction.y;
	float t5 = (bounds.min.z - start.z) / direction.z;
	float t6 = (bounds.max.z - start.z) / direction.z;

	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	if (tmin > tmax) 
	{
		return false;
	}

	float t = tmin;

	if (tmin < 0.0f) 
	{
		// Inside bounds.
		if (tmax > 0.0f)
		{
			t = tmax;
		}
		else
		{
			return false;
		}
	}

	if (intersect_point)
	{
		*intersect_point = start + t * direction;
	}

	return true;
}

}; // namespace ws
