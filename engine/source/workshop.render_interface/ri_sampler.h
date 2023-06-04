// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"

namespace ws {

// ================================================================================================
//  Represents a sampler that can be used to read from a texture within a shader.
// ================================================================================================
class ri_sampler
{
public:

    struct create_params
    {
		ri_texture_filter filter = ri_texture_filter::linear;
		
		ri_texture_address_mode address_mode_u = ri_texture_address_mode::repeat;
		ri_texture_address_mode address_mode_v = ri_texture_address_mode::repeat;
		ri_texture_address_mode address_mode_w = ri_texture_address_mode::repeat;

		ri_texture_border_color border_color = ri_texture_border_color::opaque_black;

		float min_lod = 0.0f;
		float max_lod = -1.0f;
		float mip_lod_bias = 0.0f;

		int max_anisotropy = 0;
    };

public:
    virtual ~ri_sampler() {}

	virtual ri_texture_filter get_filter() = 0;

	virtual ri_texture_address_mode get_address_mode_u() = 0;
	virtual ri_texture_address_mode get_address_mode_v() = 0;
	virtual ri_texture_address_mode get_address_mode_w() = 0;

	virtual ri_texture_border_color get_border_color() = 0;

	virtual float get_min_lod() = 0;
	virtual float get_max_lod() = 0;
	virtual float get_mip_lod_bias() = 0;

	virtual int get_max_anisotropy() = 0;

};

}; // namespace ws
