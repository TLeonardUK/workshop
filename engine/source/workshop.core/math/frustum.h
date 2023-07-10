// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/aabb.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/math/plane.h"

namespace ws {

class frustum
{
public:
	enum class intersection
	{
		outside,
		inside,
		intersects
	};

	enum class planes
	{
		left,
		right,
		top,
		bottom,
		near,
		far
	};

	enum class corner
	{
		far_top_left,
		far_top_right,
		far_bottom_left,
		far_bottom_right,
		near_top_left,
		near_top_right,
		near_bottom_left,
		near_bottom_right
	};

	static constexpr inline size_t k_plane_count = 6;
	static constexpr inline size_t k_corner_count = 8;

public:
	static const frustum Empty;

	plane planes[k_plane_count];
	vector3 corners[k_corner_count];

	frustum() = default;
	frustum(const matrix4& viewProjection);

private:
	static plane make_plane(float a, float b, float c, float d);

	void calculate_corners();

public:
	vector3 get_origin() const;
	vector3 get_direction() const;
	vector3 get_near_center() const;
	vector3 get_far_center() const;
	vector3 get_center() const;

	frustum get_cascade(float nearDistance, float farDistance);
	void get_corners(vector3 inCorners[k_corner_count]) const;

	intersection intersects(const aabb& bounds) const;
	intersection intersects(const obb& bounds) const;

};

inline plane frustum::make_plane(float a, float b, float c, float d)
{
	return plane(a, b, c, d).normalize();
}

inline void frustum::calculate_corners()
{
	corners[static_cast<int>(corner::far_top_left)] = plane::intersect(
		planes[static_cast<int>(planes::far)],
		planes[static_cast<int>(planes::top)],
		planes[static_cast<int>(planes::left)]
	);
	corners[static_cast<int>(corner::far_top_right)] = plane::intersect(
		planes[static_cast<int>(planes::far)],
		planes[static_cast<int>(planes::top)],
		planes[static_cast<int>(planes::right)]
	);
	corners[static_cast<int>(corner::far_bottom_left)] = plane::intersect(
		planes[static_cast<int>(planes::far)],
		planes[static_cast<int>(planes::bottom)],
		planes[static_cast<int>(planes::left)]
	);
	corners[static_cast<int>(corner::far_bottom_right)] = plane::intersect(
		planes[static_cast<int>(planes::far)],
		planes[static_cast<int>(planes::bottom)],
		planes[static_cast<int>(planes::right)]
	);
	corners[static_cast<int>(corner::near_top_left)] = plane::intersect(
		planes[static_cast<int>(planes::near)],
		planes[static_cast<int>(planes::top)],
		planes[static_cast<int>(planes::left)]
	);
	corners[static_cast<int>(corner::near_top_right)] = plane::intersect(
		planes[static_cast<int>(planes::near)],
		planes[static_cast<int>(planes::top)],
		planes[static_cast<int>(planes::right)]
	);
	corners[static_cast<int>(corner::near_bottom_left)] = plane::intersect(
		planes[static_cast<int>(planes::near)],
		planes[static_cast<int>(planes::bottom)],
		planes[static_cast<int>(planes::left)]
	);
	corners[static_cast<int>(corner::near_bottom_right)] = plane::intersect(
		planes[static_cast<int>(planes::near)],
		planes[static_cast<int>(planes::bottom)],
		planes[static_cast<int>(planes::right)]
	);
}

inline frustum::frustum(const matrix4& viewProjection)
{
	planes[static_cast<int>(planes::left)] = make_plane(
		viewProjection[0][3] + viewProjection[0][0],
		viewProjection[1][3] + viewProjection[1][0],
		viewProjection[2][3] + viewProjection[2][0],
		viewProjection[3][3] + viewProjection[3][0]
	);
	planes[static_cast<int>(planes::right)] = make_plane(
		viewProjection[0][3] - viewProjection[0][0],
		viewProjection[1][3] - viewProjection[1][0],
		viewProjection[2][3] - viewProjection[2][0],
		viewProjection[3][3] - viewProjection[3][0]
	);
	planes[static_cast<int>(planes::top)] = make_plane(
		viewProjection[0][3] - viewProjection[0][1],
		viewProjection[1][3] - viewProjection[1][1],
		viewProjection[2][3] - viewProjection[2][1],
		viewProjection[3][3] - viewProjection[3][1]
	);
	planes[static_cast<int>(planes::bottom)] = make_plane(
		viewProjection[0][3] + viewProjection[0][1],
		viewProjection[1][3] + viewProjection[1][1],
		viewProjection[2][3] + viewProjection[2][1],
		viewProjection[3][3] + viewProjection[3][1]
	);
	planes[static_cast<int>(planes::near)] = make_plane(
		viewProjection[0][3] + viewProjection[0][2],
		viewProjection[1][3] + viewProjection[1][2],
		viewProjection[2][3] + viewProjection[2][2],
		viewProjection[3][3] + viewProjection[3][2]
	);
	planes[static_cast<int>(planes::far)] = make_plane(
		viewProjection[0][3] - viewProjection[0][2],
		viewProjection[1][3] - viewProjection[1][2],
		viewProjection[2][3] - viewProjection[2][2],
		viewProjection[3][3] - viewProjection[3][2]
	);

	calculate_corners();
}

inline vector3 frustum::get_origin() const
{
	return plane::intersect(
		planes[static_cast<int>(planes::right)],
		planes[static_cast<int>(planes::top)],
		planes[static_cast<int>(planes::left)]
	);
}

inline vector3 frustum::get_direction() const
{
	return planes[static_cast<int>(planes::near)].get_normal();
}

inline vector3 frustum::get_near_center() const
{
	return (
		corners[static_cast<int>(corner::near_top_left)] +
		corners[static_cast<int>(corner::near_top_right)] +
		corners[static_cast<int>(corner::near_bottom_left)] +
		corners[static_cast<int>(corner::near_bottom_right)]
	) / 4.0f;
}

inline vector3 frustum::get_far_center() const
{
	return (
		corners[static_cast<int>(corner::far_top_left)] +
		corners[static_cast<int>(corner::far_top_right)] +
		corners[static_cast<int>(corner::far_bottom_left)] +
		corners[static_cast<int>(corner::far_bottom_right)]
	) / 4.0f;
}

inline vector3 frustum::get_center() const
{
	vector3 near_center = get_near_center();
	vector3 far_center = get_far_center();
	return near_center + ((far_center - near_center) * 0.5f);
}

inline frustum frustum::get_cascade(float nearDistance, float farDistance)
{
	frustum frustum;
	frustum.planes[static_cast<int>(planes::left)] = planes[static_cast<int>(planes::left)];
	frustum.planes[static_cast<int>(planes::right)] = planes[static_cast<int>(planes::right)];
	frustum.planes[static_cast<int>(planes::top)] = planes[static_cast<int>(planes::top)];
	frustum.planes[static_cast<int>(planes::bottom)] = planes[static_cast<int>(planes::bottom)];

	// This whole thing is dumb, not had a chance to sort the plane math out yet, so bruteforce ...

	vector3 origin = get_origin();

	float original_near_distance = (get_near_center() - origin).length();
	float original_far_distance = (get_far_center() - origin).length();

	plane near_plane = planes[static_cast<int>(planes::near)];
	plane far_plane = planes[static_cast<int>(planes::far)];

	frustum.planes[static_cast<int>(planes::near)] = plane(near_plane.x, near_plane.y, near_plane.z, (near_plane.w + original_near_distance) - nearDistance);
	frustum.planes[static_cast<int>(planes::far)] = plane(far_plane.x, far_plane.y, far_plane.z, (far_plane.w - original_far_distance) + farDistance);

	frustum.calculate_corners();

	return frustum;
}

inline void frustum::get_corners(vector3 inCorners[k_corner_count]) const
{
	for (int i = 0; i < k_corner_count; i++)
	{
		inCorners[i] = corners[i];
	}
}

inline frustum::intersection frustum::intersects(const aabb& bounds) const
{
	const float padding = 0.0f;

	vector3 extents = bounds.get_extents();
	vector3 center = bounds.get_center();

	bool intersecting = false;

	for (int i = 0; i < k_plane_count; i++)
	{
		vector3 abs = planes[i].get_normal().abs();

		vector3 plane_normal = planes[i].get_normal();
		float plane_distance = planes[i].get_distance();

		float r = extents.x * abs.x + extents.y * abs.y + extents.z * abs.z;
		float s = plane_normal.x * center.x + plane_normal.y * center.y + plane_normal.z * center.z;

		if (s + r < -plane_distance - padding)
		{
			return intersection::outside;
		}

		intersecting |= (s - r <= -plane_distance);
	}

	return intersecting ? intersection::intersects : intersection::inside;
}

inline frustum::intersection frustum::intersects(const obb& bounds) const
{
	const float padding = 0.0f;

	vector3 extents = bounds.get_extents();
	vector3 center = bounds.get_center();

	vector3 up = bounds.get_up_vector();
	vector3 right = bounds.get_right_vector();
	vector3 forward = bounds.get_forward_vector();

	bool intersecting = false;

	for (int i = 0; i < k_plane_count; i++)
	{
		if (i == static_cast<int>(planes::near) ||
			i == static_cast<int>(planes::far))
		{
			continue;
		}

		vector3 plane_normal = planes[i].get_normal();
		float plane_distance = planes[i].get_distance();

		float r =
			extents.x * math::abs(vector3::dot(plane_normal, right)) +
			extents.y * math::abs(vector3::dot(plane_normal, up)) +
			extents.z * math::abs(vector3::dot(plane_normal, forward));

		float s = plane_normal.x * center.x + plane_normal.y * center.y + plane_normal.z * center.z;

		if (s + r < -plane_distance - padding)
		{
			return intersection::outside;
		}

		intersecting |= (s - r <= -plane_distance);
	}

	return intersecting ? intersection::intersects : intersection::inside;
}

}; // namespace ws