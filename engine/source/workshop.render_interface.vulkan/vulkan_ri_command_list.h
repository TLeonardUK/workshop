// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_command_list.h"

namespace ws {

// ================================================================================================
//  Implementation of a command list using Vulkan.
// ================================================================================================
class vulkan_ri_command_list : public ri_command_list
{
public:
    virtual void open() override;
    virtual void close() override;

    virtual void barrier(ri_texture& resource, ri_resource_state source_state, ri_resource_state destination_state) override;
    virtual void barrier(ri_buffer& resource, ri_resource_state source_state, ri_resource_state destination_state) override;

    virtual void clear(ri_texture_view resource, const color& destination) override;
    virtual void clear_depth(ri_texture_view resource, float depth, size_t stencil) override;

    virtual void set_pipeline(ri_pipeline& pipeline) override;
    virtual void set_param_blocks(const std::vector<ri_param_block*> param_blocks) override;
    virtual void set_viewport(const recti& rect) override;
    virtual void set_scissor(const recti& rect) override;
    virtual void set_blend_factor(const vector4& factor) override;
    virtual void set_stencil_ref(uint32_t value) override;
    virtual void set_primitive_topology(ri_primitive value) override;
    virtual void set_index_buffer(ri_buffer& buffer) override;
    virtual void set_render_targets(const std::vector<ri_texture_view>& colors, ri_texture_view depth) override;

    virtual void draw(size_t indexes_per_instance, size_t instance_count, size_t start_index_location = 0) override;
    virtual void dispatch(size_t group_size_x, size_t group_size_y, size_t group_size_z) override;
    virtual void dispatch_rays(size_t group_size_x, size_t group_size_y, size_t group_size_z) override;

    virtual void begin_event(const color& color, const char* name, ...) override;
    virtual void end_event() override;

    virtual void begin_query(ri_query* query) override;
    virtual void end_query(ri_query* query) override;

    virtual void copy_texture(ri_texture* texture, ri_buffer* buffer) override;
    virtual void copy_buffer(ri_buffer* destination, ri_buffer* source) override;
    virtual void clear_buffer(ri_buffer* destination) override;

};

}; // namespace ws
