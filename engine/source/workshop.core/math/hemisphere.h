// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/aabb.h"
#include "workshop.core/math/matrix3.h"

#include <vector>

namespace ws {

template <typename element_type>
class base_hemisphere
{
public:
	using vector_type = base_vector3<element_type>;
	using matrix_type = base_matrix4<element_type>;

	vector_type origin;
	element_type radius;
	vector_type normal;

	static const base_hemisphere empty;

	base_hemisphere() = default;
	base_hemisphere(const vector_type& origin_, const vector_type& normal_, float radius_);

	matrix_type get_transform() const;
};

using hemisphere  = base_hemisphere<float>;
using hemisphered = base_hemisphere<double>;

template <typename element_type>
inline const base_hemisphere<element_type> base_hemisphere<element_type>::empty = base_hemisphere(vector_type::zero, vector_type::zero, static_cast<element_type>(0));

template <typename element_type>
inline base_hemisphere<element_type>::base_hemisphere(const vector_type& origin_, const vector_type& normal_, float radius_)
	: origin(origin_)
	, normal(normal_)
	, radius(radius_)
{
}

template <typename element_type>
base_hemisphere<element_type>::matrix_type base_hemisphere<element_type>::get_transform() const
{
	return 
		matrix_type::translate(origin) *
		matrix_type::rotation(quat::rotate_to(vector3::up, normal));
}

}; // namespace ws