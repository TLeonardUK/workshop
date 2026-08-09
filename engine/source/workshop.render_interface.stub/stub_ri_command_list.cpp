// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_command_list.h"

namespace ws {

void stub_ri_command_list::open()
{
}

void stub_ri_command_list::close()
{
}

void stub_ri_command_list::barrier(ri_texture& resource, ri_resource_state source_state, ri_resource_state destination_state)
{
}

void stub_ri_command_list::barrier(ri_buffer& resource, ri_resource_state source_state, ri_resource_state destination_state)
{
}

void stub_ri_command_list::clear(ri_texture_view resource, const color& destination)
{
}

void stub_ri_command_list::clear_depth(ri_texture_view resource, float depth, size_t stencil)
{
}

void stub_ri_command_list::set_pipeline(ri_pipeline& pipeline)
{
}

void stub_ri_command_list::set_param_blocks(const std::vector<ri_param_block*> param_blocks)
{
}

void stub_ri_command_list::set_viewport(const recti& rect)
{
}

void stub_ri_command_list::set_scissor(const recti& rect)
{
}

void stub_ri_command_list::set_blend_factor(const vector4& factor)
{
}

void stub_ri_command_list::set_stencil_ref(uint32_t value)
{
}

void stub_ri_command_list::set_primitive_topology(ri_primitive value)
{
}

void stub_ri_command_list::set_index_buffer(ri_buffer& buffer)
{
}

void stub_ri_command_list::set_render_targets(const std::vector<ri_texture_view>& colors, ri_texture_view depth)
{
}

void stub_ri_command_list::draw(size_t indexes_per_instance, size_t instance_count, size_t start_index_location)
{
}

void stub_ri_command_list::dispatch(size_t group_size_x, size_t group_size_y, size_t group_size_z)
{
}

void stub_ri_command_list::dispatch_rays(size_t group_size_x, size_t group_size_y, size_t group_size_z)
{
}

void stub_ri_command_list::begin_event(const color& color, const char* name, ...)
{
}

void stub_ri_command_list::end_event()
{
}

void stub_ri_command_list::begin_query(ri_query* query)
{
}

void stub_ri_command_list::end_query(ri_query* query)
{
}

void stub_ri_command_list::copy_texture(ri_texture* texture, ri_buffer* buffer)
{
}

}; // namespace ws
