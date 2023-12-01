// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_types.h"
#include "workshop.core/drawing/color.h"

#include <span>

namespace ws {

class ri_texture;

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
        ri_texture_dimension dimensions = ri_texture_dimension::texture_2d;
        ri_texture_format format = ri_texture_format::R8G8B8A8;
        bool is_render_target = false;

        // Number of mips in the texture (this should be the mips in the texture BEFORE drop_mips is taken into account).
        size_t mip_levels = 1;

        // If set then this texture can have rw unordered access.
        bool allow_unordered_access = false;

        // If set then you can use ri_texture_views to reference individual faces/mips
        // of the texture in-shaders.
        bool allow_individual_image_access = false;

        // Set to 0 to disable msaa.
        size_t multisample_count = 0;

        // Optimal clear value.
        color optimal_clear_color = color(0.0f, 0.0f, 0.0f, 0.0f);
        float optimal_clear_depth = 1.0f;
        uint8_t optimal_clear_stencil = 0;

        // Number of mips to drop. This is only relevant if initial data has been provided.
        size_t drop_mips = 0;

        // If texture should be partially resident.
        bool is_partially_resident = false;
        
        // How many mips should be initially resident.
        size_t resident_mips = 0;

        // Data that we will upload into the texture on construction.
        // This must be in the format returned by ri_texture_compiler.
        std::span<uint8_t> data;
    };

public:
    virtual ~ri_texture() {}

    virtual size_t get_width() = 0;
    virtual size_t get_height() = 0;
    virtual size_t get_depth() = 0;
    virtual size_t get_mip_levels() = 0;
    virtual size_t get_dropped_mips() = 0;

    virtual ri_texture_dimension get_dimensions() const = 0;
    virtual ri_texture_format get_format() = 0;

    virtual size_t get_multisample_count() = 0;

    virtual color get_optimal_clear_color() = 0;
    virtual float get_optimal_clear_depth() = 0;
    virtual uint8_t get_optimal_clear_stencil() = 0;

    virtual bool is_render_target() = 0;
    virtual bool is_depth_stencil() = 0;

    virtual bool is_partially_resident() = 0;

    virtual size_t get_resident_mips() = 0;
    virtual void make_mip_resident(size_t mip_index, const std::span<uint8_t>& linear_data) = 0;
    virtual void make_mip_non_resident(size_t mip_index) = 0;
    virtual size_t get_memory_usage_with_residency(size_t mip_count) = 0;
    virtual bool is_mip_resident(size_t mip_index) = 0;
    virtual void get_mip_source_data_range(size_t mip_index, size_t& offset, size_t& size) = 0;

    virtual ri_resource_state get_initial_state() = 0;

    virtual const char* get_debug_name() = 0;

    virtual void swap(ri_texture* other) = 0;

};

// ================================================================================================
//  Represents a view to a specific part of a texture.
// ================================================================================================
struct ri_texture_view
{
public:
    ri_texture_view() = default;

    ri_texture_view(ri_texture* in_texture)
        : texture(in_texture)
    {
    }

    ri_texture_view(ri_texture* in_texture, size_t in_slice)
        : texture(in_texture)
        , slice(in_slice)
    {
    }

    ri_texture_view(ri_texture* in_texture, size_t in_slice, size_t in_mip)
        : texture(in_texture)
        , slice(in_slice)
        , mip(in_mip)
    {
    }

public:

    // Gets width/height taking mip level into account.

    size_t get_width()
    {
        size_t full_size = texture->get_width();

        size_t mip_index = mip;
        if (mip_index == k_unset)
        {
            mip_index = 0;
        }

        for (size_t i = mip_index; i > 0; i--)
        {
            full_size /= 2;
        }

        return full_size;
    }

    size_t get_height()
    {
        size_t full_size = texture->get_height();
        
        size_t mip_index = mip;
        if (mip_index == k_unset)
        {
            mip_index = 0;
        }
        
        for (size_t i = mip_index; i > 0; i--)
        {
            full_size /= 2;
        }

        return full_size;
    }

public:

    inline static constexpr size_t k_unset = std::numeric_limits<size_t>::max();

    ri_texture* texture = nullptr;
    size_t slice = k_unset;
    size_t mip = k_unset;

};

}; // namespace ws
