// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/aabb.h"
#include "workshop.core/math/matrix4.h"

namespace ws {

class obb
{
public:
	aabb bounds;
	matrix4 transform;

	obb() = default;
	obb(const aabb& bounds_, const matrix4& transform_);

	vector3 get_center() const;
	vector3 get_extents() const;

	vector3 get_up_vector() const;
	vector3 get_right_vector() const;
	vector3 get_forward_vector() const;

	void get_corners(vector3 corners[aabb::k_corner_count]) const;
	aabb get_aligned_bounds() const;

};

inline obb::obb(const aabb& bounds_, const matrix4& transform_)
	: bounds(bounds_)
	, transform(transform_)
{
}

inline vector3 obb::get_center() const
{
	return bounds.get_center() * transform;
}

inline vector3 obb::get_extents() const
{
	// This is lossy. Better bet is to have scale baked in bounds and have transform
	// be purely location/rotation?.
	return bounds.get_extents() * transform.extract_scale();
}

inline vector3 obb::get_up_vector() const
{
	return transform.transform_direction(vector3::up);
}

inline vector3 obb::get_right_vector() const
{
	return transform.transform_direction(vector3::right);
}

inline vector3 obb::get_forward_vector() const
{
	return transform.transform_direction(vector3::forward);
}

inline void obb::get_corners(vector3 corners[aabb::k_corner_count]) const
{
	bounds.get_corners(corners);

	for (int i = 0; i < aabb::k_corner_count; i++)
	{
		corners[i] = corners[i] * transform;
	}
}

inline aabb obb::get_aligned_bounds() const
{
	vector3 worldCorners[aabb::k_corner_count];
	bounds.get_corners(worldCorners);

	std::vector<vector3> points;
	points.reserve(8);

	for (int i = 0; i < aabb::k_corner_count; i++)
	{
		points.push_back(transform.transform_location(worldCorners[i]));
	}

	return aabb(points);
}

}; // namespace ws