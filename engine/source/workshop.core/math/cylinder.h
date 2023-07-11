// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/matrix3.h"

namespace ws {

template <typename element_type>
class base_cylinder
{
public:
	using vector_type = base_vector3<element_type>;
	using matrix_type = base_matrix4<element_type>;

	vector_type origin;
	vector_type up;
	element_type radius;
	element_type height;

	static const base_cylinder empty;

	base_cylinder() = default;
	base_cylinder(const vector_type& origin_, const vector_type& up_, element_type radius_, element_type height_);

	matrix_type get_transform() const;
};

using cylinder  = base_cylinder<float>;
using cylinderd = base_cylinder<double>;

template <typename element_type>
inline const base_cylinder<element_type> base_cylinder<element_type>::empty = base_cylinder(vector_type::zero, vector_type::zero, stati_cast<element_type>(0), static_cast<element_type>(0));

template <typename element_type>
inline base_cylinder<element_type>::base_cylinder(const vector_type& origin_, const vector_type& up_, element_type radius_, element_type height_)
	: origin(origin_)
	, up(up_)
	, radius(radius_)
	, height(height_)
{
}

template <typename element_type>
base_cylinder<element_type>::matrix_type base_cylinder<element_type>::get_transform() const
{
	return matrix_type::translate(origin) * 
		   matrix_type::rotation(quat::rotate_to(vector3::up, up));
}

}; // namespace ws