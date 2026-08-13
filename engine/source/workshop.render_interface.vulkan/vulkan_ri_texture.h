// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_texture.h"

#include <string>

namespace ws {

// ================================================================================================
//  Implementation of a texture buffer using Vulkan.
// ================================================================================================
class vulkan_ri_texture : public ri_texture
{
public:
    vulkan_ri_texture(const create_params& params, const char* debug_name);

    virtual size_t get_width() override;
    virtual size_t get_pitch() override;
    virtual size_t get_height() override;
    virtual size_t get_depth() override;
    virtual size_t get_mip_levels() override;
    virtual size_t get_dropped_mips() override;

    virtual ri_texture_dimension get_dimensions() const override;
    virtual ri_texture_format get_format() override;

    virtual size_t get_multisample_count() override;

    virtual color get_optimal_clear_color() override;
    virtual float get_optimal_clear_depth() override;
    virtual uint8_t get_optimal_clear_stencil() override;

    virtual bool is_render_target() override;
    virtual bool is_depth_stencil() override;

    virtual bool is_partially_resident() const override;

    virtual size_t get_resident_mips() override;
    virtual void make_mip_resident(size_t mip_index, const std::span<uint8_t>& linear_data) override;
    virtual void make_mip_resident(size_t mip_index, ri_staging_buffer& data_buffer) override;
    virtual void make_mip_non_resident(size_t mip_index) override;
    virtual size_t get_memory_usage_with_residency(size_t mip_count) override;
    virtual bool is_mip_resident(size_t mip_index) override;
    virtual void get_mip_source_data_range(size_t mip_index, size_t& offset, size_t& size) override;
    virtual void begin_mip_residency_change() override;
    virtual void end_mip_residency_change() override;

    virtual ri_resource_state get_initial_state() override;

    virtual const char* get_debug_name() override;

    virtual void swap(ri_texture* other) override;

private:
    create_params m_params;
    std::string m_debug_name;

};

}; // namespace ws
