// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/texture/texture.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"

namespace ws {

texture::texture(ri_interface& ri_interface, renderer& renderer)
    : m_ri_interface(ri_interface)
    , m_renderer(renderer)
{
}

texture::~texture()
{
}

bool texture::post_load()
{
    ri_texture::create_params params;
    params.width = width;
    params.height = height;
    params.depth = depth;
    params.format = ri_convert_pixmap_format(format);
    params.dimensions = dimensions;
    params.is_render_target = false;
    params.mip_levels = mip_levels;
    params.data = data;

    ri_instance = m_ri_interface.create_texture(params, name.c_str());
    if (!ri_instance)
    {
        db_error(asset, "Failed to create texture '%s'.", name.c_str());
        return false;
    }

    // No need to keep the local data around any more, the RI interface will have copied 
    // it to the gpu now.
    data.clear();

    return true;
}

}; // namespace ws
