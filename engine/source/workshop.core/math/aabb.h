// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/utils/yaml.h"

#include <vector>

namespace ws {

class aabb
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
	static const aabb zero;

	vector3 min;
	vector3 max;

	aabb() = default;
	aabb(const vector3& in_min, const vector3& in_max);
	aabb(const vector3* points, size_t count);
	aabb(const std::vector<vector3>& points);
	
	static aabb from_center_and_extents(const vector3& center, const vector3& extents);

	vector3 get_center() const;
	vector3 get_extents() const;
	void get_corners(vector3 corners[k_corner_count]) const;

	void subdivide(aabb divisions[8]) const;
	bool intersects(const aabb& other) const;
	bool contains(const aabb& other) const;
    aabb combine(const aabb& other) const;
};

inline const aabb aabb::zero = aabb(vector3(0.0f, 0.0f, 0.0f), vector3(0.0f, 0.0f, 0.0f));

inline aabb::aabb(const vector3& in_min, const vector3& in_max)
	: min(in_min)
	, max(in_max)
{
}

inline aabb::aabb(const vector3* points, size_t count)
{
	if (count > 0)
	{
		min = points[0];
		max = points[0];

		for (size_t i = 0; i < count; i++)
		{
			const vector3& point = points[i];

			min = vector3::min(min, point);
			max = vector3::max(max, point);
		}
	}
	else
	{
		min = vector3::zero;
		max = vector3::zero;
	}
}

inline aabb::aabb(const std::vector<vector3>& points)
	: aabb(points.data(), points.size())
{
}


inline aabb aabb::from_center_and_extents(const vector3& center, const vector3& extents)
{
	return aabb(center - extents, center + extents);
}

inline vector3 aabb::get_center() const
{
	return vector3(
		min.x + ((max.x - min.x) * 0.5f),
		min.y + ((max.y - min.y) * 0.5f),
		min.z + ((max.z - min.z) * 0.5f)
	);
}

inline vector3 aabb::get_extents() const
{
	return vector3(
		(max.x - min.x) * 0.5f,
		(max.y - min.y) * 0.5f,
		(max.z - min.z) * 0.5f
	);
}

inline void aabb::get_corners(vector3 corners[k_corner_count]) const
{
	corners[static_cast<int>(corner::front_top_left)] = vector3(min.x, max.y, min.z);
	corners[static_cast<int>(corner::front_top_right)] = vector3(max.x, max.y, min.z);
	corners[static_cast<int>(corner::front_bottom_left)] = vector3(min.x, min.y, min.z);
	corners[static_cast<int>(corner::front_bottom_right)] = vector3(max.x, min.y, min.z);
	corners[static_cast<int>(corner::back_top_left)] = vector3(min.x, max.y, max.z);
	corners[static_cast<int>(corner::back_top_right)] = vector3(max.x, max.y, max.z);
	corners[static_cast<int>(corner::back_bottom_left)] = vector3(min.x, min.y, max.z);
	corners[static_cast<int>(corner::back_bottom_right)] = vector3(max.x, min.y, max.z);
}

inline void aabb::subdivide(aabb divisions[8]) const
{
	vector3 center = get_center();

	divisions[0] = aabb(vector3(min.x, min.y, min.z), vector3(center.x, center.y, center.z));
	divisions[1] = aabb(vector3(min.x, center.y, min.z), vector3(center.x, max.y, center.z));
	divisions[2] = aabb(vector3(center.x, min.y, min.z), vector3(max.x, center.y, center.z));
	divisions[3] = aabb(vector3(center.x, center.y, min.z), vector3(max.x, max.y, center.z));
	divisions[4] = aabb(vector3(min.x, min.y, center.z), vector3(center.x, center.y, max.z));
	divisions[5] = aabb(vector3(min.x, center.y, center.z), vector3(center.x, max.y, max.z));
	divisions[6] = aabb(vector3(center.x, min.y, center.z), vector3(max.x, center.y, max.z));
	divisions[7] = aabb(vector3(center.x, center.y, center.z), vector3(max.x, max.y, max.z));
}

inline bool aabb::intersects(const aabb& other) const
{
	if (max.x < other.min.x ||
		max.y < other.min.y ||
		max.z < other.min.z ||
		min.x > other.max.x ||
		min.y > other.max.y ||
		min.z > other.max.z)
	{
		return false;
	}

	return true;
}

inline bool aabb::contains(const aabb& other) const
{
	if ((other.min.x >= min.x && other.max.x <= max.x) &&
		(other.min.y >= min.y && other.max.y <= max.y) &&
		(other.min.z >= min.z && other.max.z <= max.z))
	{
		return true;
	}

	return false;
}

inline aabb aabb::combine(const aabb& other) const
{
    vector3 cmin = vector3::min(min, other.min);
    vector3 cmax = vector3::max(max, other.max);
    return aabb(cmin, cmax);
}

template<>
inline void stream_serialize(stream& out, aabb& v)
{
	stream_serialize(out, v.min);
	stream_serialize(out, v.max);
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, aabb& value)
{
	YAML::Node min = out["min"];
	YAML::Node max = out["max"];

	yaml_serialize(min, is_loading, value.min);
	yaml_serialize(max, is_loading, value.max);
}

}; // namespace ws
