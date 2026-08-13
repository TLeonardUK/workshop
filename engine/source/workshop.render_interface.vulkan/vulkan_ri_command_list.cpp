// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"

namespace ws {

void vulkan_ri_command_list::open()
{
}

void vulkan_ri_command_list::close()
{
}

void vulkan_ri_command_list::barrier(ri_texture& resource, ri_resource_state source_state, ri_resource_state destination_state)
{
}

void vulkan_ri_command_list::barrier(ri_buffer& resource, ri_resource_state source_state, ri_resource_state destination_state)
{
}

void vulkan_ri_command_list::clear(ri_texture_view resource, const color& destination)
{
}

void vulkan_ri_command_list::clear_depth(ri_texture_view resource, float depth, size_t stencil)
{
}

void vulkan_ri_command_list::set_pipeline(ri_pipeline& pipeline)
{
}

void vulkan_ri_command_list::set_param_blocks(const std::vector<ri_param_block*> param_blocks)
{
}

void vulkan_ri_command_list::set_viewport(const recti& rect)
{
}

void vulkan_ri_command_list::set_scissor(const recti& rect)
{
}

void vulkan_ri_command_list::set_blend_factor(const vector4& factor)
{
}

void vulkan_ri_command_list::set_stencil_ref(uint32_t value)
{
}

void vulkan_ri_command_list::set_primitive_topology(ri_primitive value)
{
}

void vulkan_ri_command_list::set_index_buffer(ri_buffer& buffer)
{
}

void vulkan_ri_command_list::set_render_targets(const std::vector<ri_texture_view>& colors, ri_texture_view depth)
{
}

void vulkan_ri_command_list::draw(size_t indexes_per_instance, size_t instance_count, size_t start_index_location)
{
}

void vulkan_ri_command_list::dispatch(size_t group_size_x, size_t group_size_y, size_t group_size_z)
{
}

void vulkan_ri_command_list::dispatch_rays(size_t group_size_x, size_t group_size_y, size_t group_size_z)
{
}

void vulkan_ri_command_list::begin_event(const color& color, const char* name, ...)
{
}

void vulkan_ri_command_list::end_event()
{
}

void vulkan_ri_command_list::begin_query(ri_query* query)
{
}

void vulkan_ri_command_list::end_query(ri_query* query)
{
}

void vulkan_ri_command_list::copy_texture(ri_texture* texture, ri_buffer* buffer)
{
}

void vulkan_ri_command_list::copy_buffer(ri_buffer* destination, ri_buffer* source)
{
}

void vulkan_ri_command_list::clear_buffer(ri_buffer* destination)
{
}

}; // namespace ws
