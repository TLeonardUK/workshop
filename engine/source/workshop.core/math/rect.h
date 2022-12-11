// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

namespace ws {

template <typename element_type>
class base_rect
{
public:
	element_type x, y, width, height;

	static const base_rect empty;

	base_rect() = default;	
	base_rect(element_type x, element_type y, element_type w, element_type h);
};

using rect = base_rect<float>;
using recti = base_rect<int>;
using rectu = base_rect<unsigned int>;
using rectd = base_rect<double>;

template <typename element_type>
inline const base_rect<element_type> base_rect<element_type>::empty(static_cast<element_type>(0), static_cast<element_type>(0), static_cast<element_type>(0), static_cast<element_type>(0));

template <typename element_type>
inline base_rect<element_type>::base_rect(element_type _x, element_type _y, element_type _w, element_type _h)
	: x(_x)
	, y(_y)
	, width(_w)
	, height(_h)
{
}

template <typename element_type>
inline bool operator==(const base_rect<element_type>& first, const base_rect<element_type>& second)
{
	return 
		first.x == second.x &&
		first.y == second.y &&
		first.width == second.width &&
		first.height == second.height;
}

template <typename element_type>
inline bool operator!=(const base_rect<element_type>& first, const base_rect<element_type>& second)
{
	return !(first == second);
}

}; // namespace ws