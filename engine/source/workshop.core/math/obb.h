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
    enum class corner
    {
        front_top_left,
        front_top_right,
        front_bottom_left,
        front_bottom_right,
        back_top_left,
        back_top_right,
        back_bottom_left,
        back_bottom_right
    };

    static constexpr inline size_t k_corner_count = 8;

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

	void get_corners(vector3 corners[obb::k_corner_count]) const;
	aabb get_aligned_bounds() const;

    vector3 get_closest_point(const vector3& other) const;

    obb combine(const obb& other) const;
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
	return transform.transform_direction(vector3::up).normalize();      // TODO: Fix this, direction transformation shouldn't take scale into account.
}

inline vector3 obb::get_right_vector() const
{
	return transform.transform_direction(vector3::right).normalize();
}

inline vector3 obb::get_forward_vector() const
{
	return transform.transform_direction(vector3::forward).normalize();
}

inline void obb::get_corners(vector3 corners[obb::k_corner_count]) const
{
	bounds.get_corners(corners);

	for (int i = 0; i < obb::k_corner_count; i++)
	{
		corners[i] = corners[i] * transform;
	}
}

inline aabb obb::get_aligned_bounds() const
{
	vector3 worldCorners[obb::k_corner_count];
	bounds.get_corners(worldCorners);

	std::vector<vector3> points;
	points.reserve(8);

	for (int i = 0; i < obb::k_corner_count; i++)
	{
		points.push_back(transform.transform_location(worldCorners[i]));
	}

	return aabb(points);
}

inline obb obb::combine(const obb& other) const
{
    aabb this_aabb = get_aligned_bounds();
    aabb other_aabb = other.get_aligned_bounds();
    return obb(this_aabb.combine(other_aabb), matrix4::identity);
}

inline vector3 obb::get_closest_point(const vector3& other) const
{
    // From realtime collision detection.

    vector3 e = get_extents();
    vector3 c = get_center();
    vector3 d = other - c;
    vector3 q = c;

    vector3 axis[3] = {
        get_right_vector(),
        get_up_vector(),
        get_forward_vector()
    };

    for (size_t i = 0; i < 3; i++)
    {
        float dist = vector3::dot(d, axis[i]);
        if (dist > e[i]) 
        {
            dist = e[i];
        }
        if (dist < -e[i])
        {
            dist = -e[i];
        }
        q += dist * axis[i];
    }

    return q;
}

}; // namespace ws