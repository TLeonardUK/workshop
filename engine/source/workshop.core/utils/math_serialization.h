// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/yaml.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/math/vector2.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/vector4.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/aabb.h"
#include "workshop.core/drawing/color.h"

namespace ws {

template<>
inline void stream_serialize(stream& out, vector2& v)
{
	stream_serialize(out, v.x);
	stream_serialize(out, v.y);
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, vector2& value)
{
	YAML::Node x = out["x"];
	YAML::Node y = out["y"];

	yaml_serialize(x, is_loading, value.x);
	yaml_serialize(y, is_loading, value.y);
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

template<>
inline void stream_serialize(stream& out, vector4& v)
{
	stream_serialize(out, v.x);
	stream_serialize(out, v.y);
	stream_serialize(out, v.z);
	stream_serialize(out, v.w);
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, vector4& value)
{
	YAML::Node x = out["x"];
	YAML::Node y = out["y"];
	YAML::Node z = out["z"];
	YAML::Node w = out["w"];

	yaml_serialize(x, is_loading, value.x);
	yaml_serialize(y, is_loading, value.y);
	yaml_serialize(z, is_loading, value.z);
	yaml_serialize(w, is_loading, value.w);
}

template<>
inline void stream_serialize(stream& out, quat& v)
{
	stream_serialize(out, v.x);
	stream_serialize(out, v.y);
	stream_serialize(out, v.z);
	stream_serialize(out, v.w);
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, quat& value)
{
	YAML::Node x = out["x"];
	YAML::Node y = out["y"];
	YAML::Node z = out["z"];
	YAML::Node w = out["w"];

	yaml_serialize(x, is_loading, value.x);
	yaml_serialize(y, is_loading, value.y);
	yaml_serialize(z, is_loading, value.z);
	yaml_serialize(w, is_loading, value.w);
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

template<>
inline void stream_serialize(stream& out, color& v)
{
    stream_serialize(out, v.r);
    stream_serialize(out, v.g);
    stream_serialize(out, v.b);
    stream_serialize(out, v.a);
}

template<>
inline void yaml_serialize(YAML::Node& out, bool is_loading, color& value)
{
    YAML::Node r = out["r"];
    YAML::Node g = out["g"];
    YAML::Node b = out["b"];
    YAML::Node a = out["a"];

    yaml_serialize(r, is_loading, value.r);
    yaml_serialize(g, is_loading, value.g);
    yaml_serialize(b, is_loading, value.b);
    yaml_serialize(a, is_loading, value.a);
}

}; // namespace ws
