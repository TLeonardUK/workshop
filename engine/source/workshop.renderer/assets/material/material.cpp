// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/material/material.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.assets/asset_manager.h"

namespace ws {

material::material(ri_interface& ri_interface, renderer& renderer, asset_manager& ass_manager)
    : m_ri_interface(ri_interface)
    , m_renderer(renderer)
    , m_asset_manager(ass_manager)
{
}

material::~material()
{
}

bool material::post_load()
{
    for (sampler_info& info : samplers)
    {
        ri_sampler::create_params params;
        params.filter = info.filter;
        params.address_mode_u = info.address_mode_u;
        params.address_mode_v = info.address_mode_v;
        params.address_mode_w = info.address_mode_w;
        params.border_color = info.border_color;
        params.min_lod = info.min_lod;
        params.max_lod = info.max_lod;
        params.mip_lod_bias = info.mip_lod_bias;
        params.max_anisotropy = info.max_anisotropy;

        info.ri_sampler = m_ri_interface.create_sampler(params, info.name.c_str());
    }

    for (texture_info& info : textures)
    {
        info.texture = m_asset_manager.request_asset<texture>(info.path.c_str(), 0);
    }

    return true;
}

ri_sampler* material::get_sampler(const char* name, ri_sampler* default_instance)
{
    for (sampler_info& info : samplers)
    {
        if (info.name == name)
        {
            return info.ri_sampler.get();
        }
    }
    return default_instance;
}

ri_texture* material::get_texture(const char* name, ri_texture* default_instance)
{
    for (texture_info& info : textures)
    {
        if (info.name == name && info.texture.is_loaded())
        {
            return info.texture->ri_instance.get();
        }
    }
    return default_instance;
}

}; // namespace ws
