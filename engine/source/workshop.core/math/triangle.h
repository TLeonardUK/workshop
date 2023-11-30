// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/vector2.h"
#include "workshop.core/math/math.h"

namespace ws {

template <typename vector_type>
class base_triangle
{
public:
	vector_type a = vector_type::zero;
	vector_type b = vector_type::zero;
	vector_type c = vector_type::zero;

    base_triangle() = default;
    base_triangle(const vector_type& a, const vector_type& b, const vector_type& c);

    float get_area();
};

using triangle = base_triangle<vector3>;
using triangle2d = base_triangle<vector2>;

template <typename vector_type>
inline base_triangle<vector_type>::base_triangle(const vector_type& in_a, const vector_type& in_b, const vector_type& in_c)
{
	a = in_a;
	b = in_b;
	c = in_c;
}

template <typename vector_type>
inline float base_triangle<vector_type>::get_area()
{
    float len_a = (a - b).length();
    float len_b = (b - c).length();
    float len_c = (c - a).length();

    float s = (len_a + len_b + len_c) * 0.5f;
    return sqrt(s * (s - len_a) * (s - len_b) * (s - len_c));
}

}; // namespace ws
