// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_sampler.h"

namespace ws {

// ================================================================================================
//  Implementation of a texture sampler using Vulkan.
// ================================================================================================
class vulkan_ri_sampler : public ri_sampler
{
public:
    vulkan_ri_sampler(const create_params& params, const char* debug_name);

    virtual ri_texture_filter get_filter() override;

    virtual ri_texture_address_mode get_address_mode_u() override;
    virtual ri_texture_address_mode get_address_mode_v() override;
    virtual ri_texture_address_mode get_address_mode_w() override;

    virtual ri_texture_border_color get_border_color() override;

    virtual float get_min_lod() override;
    virtual float get_max_lod() override;
    virtual float get_mip_lod_bias() override;

    virtual int get_max_anisotropy() override;

private:
    create_params m_params;

};

}; // namespace ws
