// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_sampler.h"

namespace ws {

stub_ri_sampler::stub_ri_sampler(const create_params& params, const char* debug_name)
    : m_params(params)
{
}

ri_texture_filter stub_ri_sampler::get_filter()
{
    return m_params.filter;
}

ri_texture_address_mode stub_ri_sampler::get_address_mode_u()
{
    return m_params.address_mode_u;
}

ri_texture_address_mode stub_ri_sampler::get_address_mode_v()
{
    return m_params.address_mode_v;
}

ri_texture_address_mode stub_ri_sampler::get_address_mode_w()
{
    return m_params.address_mode_w;
}

ri_texture_border_color stub_ri_sampler::get_border_color()
{
    return m_params.border_color;
}

float stub_ri_sampler::get_min_lod()
{
    return m_params.min_lod;
}

float stub_ri_sampler::get_max_lod()
{
    return m_params.max_lod;
}

float stub_ri_sampler::get_mip_lod_bias()
{
    return m_params.mip_lod_bias;
}

int stub_ri_sampler::get_max_anisotropy()
{
    return m_params.max_anisotropy;
}

}; // namespace ws
