// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"
#include "workshop.core/drawing/color.h"

#include <span>

namespace ws {

// ================================================================================================
//  Represents a block of texture memory, which can optionally be flagged for use
//  as a render target.
// ================================================================================================
class ri_texture
{
public:

    struct create_params
    {
        size_t width = 1;
        size_t height = 1;
        size_t depth = 1;
        size_t mip_levels = 0;
        ri_texture_dimension dimensions = ri_texture_dimension::texture_2d;
        ri_texture_format format = ri_texture_format::R8G8B8A8;
        bool is_render_target = false;

        // Set to 0 to disable msaa.
        size_t multisample_count = 0;

        // Optimal clear value.
        color optimal_clear_color = color(0.0f, 0.0f, 0.0f, 0.0f);
        float optimal_clear_depth = 0.0f;
        uint8_t optimal_clear_stencil = 0;

        // Linear data that we will upload into the texture on construction.
        std::span<uint8_t> linear_data;
    };

public:

    virtual size_t get_width() = 0;
    virtual size_t get_height() = 0;
    virtual size_t get_depth() = 0;
    virtual size_t get_mip_levels() = 0;
    
    virtual ri_texture_dimension get_dimensions() const = 0;
    virtual ri_texture_format get_format() = 0;

    virtual size_t get_multisample_count() = 0;

    virtual color get_optimal_clear_color() = 0;
    virtual float get_optimal_clear_depth() = 0;
    virtual uint8_t get_optimal_clear_stencil() = 0;

    virtual bool is_render_target() = 0;

    virtual ri_resource_state get_initial_state() = 0;


};

}; // namespace ws
