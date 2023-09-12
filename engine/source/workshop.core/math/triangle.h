// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/math.h"

namespace ws {

class triangle
{
public:
	vector3 a = vector3::zero;
	vector3 b = vector3::zero;
	vector3 c = vector3::zero;

	triangle() = default;
	triangle(const vector3& a, const vector3& b, const vector3& c);
};

inline triangle::triangle(const vector3& in_a, const vector3& in_b, const vector3& in_c)
{
	a = in_a;
	b = in_b;
	c = in_c;
}

}; // namespace ws
