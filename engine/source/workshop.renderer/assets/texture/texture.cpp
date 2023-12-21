// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/texture/texture.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_cvars.h"
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
    if (streamed && streaming_info)
    {
        m_renderer.get_texture_streamer().unregister_texture(this);
    }
}

bool texture::load_dependencies()
{
    render_texture_streamer& streamer = m_renderer.get_texture_streamer();

    ri_texture::create_params params;
    params.width = width;
    params.height = height;
    params.depth = depth;
    params.format = ri_convert_pixmap_format(format);
    params.dimensions = dimensions;
    params.is_render_target = false;
    params.data = data;

    if (std::max(params.width, params.height) < cvar_texture_streaming_min_dimension.get_int())
    {
        db_warning(renderer, "Disabled streaming for texture as it is smaller than minimum dimensions, consider turning off streaming in the asset: %s", name.c_str());
        streamed = false;
    }

    if (params.dimensions != ri_texture_dimension::texture_2d)
    {
        db_warning(renderer, "Disabled streaming for texture as only 2d textures support streaming, consider turning off streaming in the asset: %s", name.c_str());
        streamed = false;
    }

#if 1
    // DEBUG DEBUG DEBUG
    //if (name.find("wood_tile") == std::string::npos)
    if (name.find("lion") == std::string::npos)
    {
    //    streamed = false;
    }
    // DEBUG DEBUG DEBUG
#endif

    if (streamed)
    {
        params.is_partially_resident = true;
        params.mip_levels = mip_levels;
        params.resident_mips = streamer.get_current_resident_mip_count(this);
        params.drop_mips = cvar_textures_dropped_mips.get_int();
    }
    else
    {
        params.is_partially_resident = false;
        params.mip_levels = mip_levels;
        params.resident_mips = mip_levels;
        params.drop_mips = cvar_textures_dropped_mips.get_int();
    }

    ri_instance = m_ri_interface.create_texture(params, name.c_str());
    if (!ri_instance)
    {
        db_error(asset, "Failed to create texture '%s'.", name.c_str());
        return false;
    }

    // No need to keep the local data around any more, the RI interface will have copied 
    // it to the gpu now.
    data.clear();
    data.shrink_to_fit();

    return true;
}

bool texture::post_load()
{
    if (streamed)
    {
        m_renderer.get_texture_streamer().register_texture(this);
    }

    return true;
}

void texture::swap(texture* other)
{
    std::swap(usage, other->usage);
    std::swap(dimensions, other->dimensions);
    std::swap(format, other->format);
    std::swap(width, other->width);
    std::swap(height, other->height);
    std::swap(depth, other->depth);
    std::swap(mipmapped, other->mipmapped);
    std::swap(streamed, other->streamed);
    std::swap(faces, other->faces);
    std::swap(mip_levels, other->mip_levels);
    std::swap(data, other->data);

    ri_instance->swap(other->ri_instance.get());
}

}; // namespace ws
