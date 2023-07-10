// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_debug.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_callback.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"

#include "workshop.core/math/matrix4.h"
#include "workshop.core/perf/profile.h"

#include "workshop.render_interface/ri_interface.h"

// Note:
// Some of the geometric creation is based on code in the directxtk12 Geometry.cpp file.

namespace ws {

render_system_debug::render_system_debug(renderer& render)
    : render_system(render, "debug")
{
}

void render_system_debug::register_init(init_list& list)
{
}

void render_system_debug::create_graph(render_graph& graph)
{
    std::unique_ptr<render_pass_callback> pass = std::make_unique<render_pass_callback>();
    pass->callback = std::bind(&render_system_debug::generate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    m_render_pass = pass.get();

    graph.add_node(std::move(pass));
}

void render_system_debug::step(const render_world_state& state)
{
    ri_interface& ri = m_renderer.get_render_interface();

    // Test
    //add_aabb(aabb(vector3(-80.0f, -80.0f, -80.0f), vector3(80.0f, 80.0f, 80.0f)), color::green);

    add_sphere(sphere(vector3::zero, 30.0f), color::red);

    if (m_vertices.empty())
    {
        return;
    }

    // Make sure vertex buffer has enough space.
    if (m_vertex_buffer == nullptr || m_vertex_buffer->get_element_count() < m_vertices.size())
    {
        ri_buffer::create_params params;
        params.element_count = m_vertices.size();
        params.element_size = sizeof(debug_primitive_vertex);
        params.usage = ri_buffer_usage::vertex_buffer;

        m_vertex_buffer = ri.create_buffer(params, "Debug Primitive Vertex Buffer");
    }

    // Make sure index buffer has enough space.
    if (m_index_buffer == nullptr || m_index_buffer->get_element_count() < m_vertices.size())
    {
        ri_buffer::create_params params;
        params.element_count = m_vertices.size();
        params.element_size = sizeof(uint16_t);
        params.usage = ri_buffer_usage::index_buffer;

        m_index_buffer = ri.create_buffer(params, "Debug Primitive Index Buffer");

        // Update index buffer.
        uint16_t* index_ptr = reinterpret_cast<uint16_t*>(m_index_buffer->map(0, m_vertices.size() * sizeof(uint16_t)));
        for (size_t i = 0; i < m_vertices.size(); i++)
        {
            index_ptr[i] = static_cast<uint16_t>(i);
        }
        m_index_buffer->unmap(index_ptr);
    }

    // Update vertex buffer.
    debug_primitive_vertex* vertex_ptr = reinterpret_cast<debug_primitive_vertex*>(m_vertex_buffer->map(0, m_vertices.size() * sizeof(debug_primitive_vertex)));
    for (size_t i = 0; i < m_vertices.size(); i++)
    {
        vertex_ptr[i] = m_vertices[i];
    }
    m_vertex_buffer->unmap(vertex_ptr);    

    // Reset vertex buffer so we can start filling it again.
    m_draw_vertex_count = m_vertices.size();
    m_vertices.clear();
}

void render_system_debug::generate(renderer& renderer, render_pass::generated_state& state_output, render_view& view)
{
    if (m_draw_vertex_count == 0)
    {
        return;
    }


    render_output output;
    output.color_targets = m_renderer.get_swapchain_output().color_targets;
    output.depth_target = m_renderer.get_gbuffer_output().depth_target;

    render_effect::technique* technique = m_renderer.get_effect_manager().get_technique("render_debug_primitive", {});

    // TODO: Cache all the param blocks.

    std::unique_ptr<ri_param_block> vertex_info_param_block = renderer.get_param_block_manager().create_param_block("vertex_info");
    vertex_info_param_block->set("vertex_buffer", *m_vertex_buffer);
    vertex_info_param_block->set("vertex_buffer_offset", 0u);
    vertex_info_param_block->clear_buffer("instance_buffer");

    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        profile_gpu_marker(list, profile_colors::gpu_pass, "debug");

        list.barrier(*output.color_targets[0], ri_resource_state::initial, ri_resource_state::render_target);
        list.barrier(*output.depth_target, ri_resource_state::initial, ri_resource_state::depth_read);
        list.set_pipeline(*technique->pipeline.get());
        list.set_render_targets(output.color_targets, output.depth_target);
        list.set_viewport(view.get_viewport());
        list.set_scissor(view.get_viewport());
        list.set_primitive_topology(ri_primitive::line_list);
        list.set_index_buffer(*m_index_buffer);
        list.set_param_blocks({ 
            vertex_info_param_block.get(), 
            view.get_view_info_param_block()
        });
        list.draw(m_draw_vertex_count, 1, 0);

        list.barrier(*output.depth_target, ri_resource_state::depth_read, ri_resource_state::initial);
        list.barrier(*output.color_targets[0], ri_resource_state::render_target, ri_resource_state::initial);
        
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

void render_system_debug::add_line(vector3 start, vector3 end, color color)
{
    debug_primitive_vertex& v0 = m_vertices.emplace_back();
    v0.position = start;
    v0.color = color.argb();

    debug_primitive_vertex& v1 = m_vertices.emplace_back();
    v1.position = end;
    v1.color = color.argb();
}

void render_system_debug::add_aabb(aabb bounds, color color)
{
    vector3 corners[aabb::k_corner_count];
    bounds.get_corners(corners);
    
    add_line(corners[(int)aabb::corner::back_top_left], corners[(int)aabb::corner::back_top_right], color);
    add_line(corners[(int)aabb::corner::front_top_left], corners[(int)aabb::corner::front_top_right], color);
    add_line(corners[(int)aabb::corner::back_top_left], corners[(int)aabb::corner::front_top_left], color);
    add_line(corners[(int)aabb::corner::back_top_right], corners[(int)aabb::corner::front_top_right], color);
    add_line(corners[(int)aabb::corner::back_bottom_left], corners[(int)aabb::corner::back_bottom_right], color);
    add_line(corners[(int)aabb::corner::front_bottom_left], corners[(int)aabb::corner::front_bottom_right], color);
    add_line(corners[(int)aabb::corner::back_bottom_left], corners[(int)aabb::corner::front_bottom_left], color);
    add_line(corners[(int)aabb::corner::back_bottom_right], corners[(int)aabb::corner::front_bottom_right], color);
    add_line(corners[(int)aabb::corner::back_top_left], corners[(int)aabb::corner::back_bottom_left], color);
    add_line(corners[(int)aabb::corner::back_top_right], corners[(int)aabb::corner::back_bottom_right], color);
    add_line(corners[(int)aabb::corner::front_top_left], corners[(int)aabb::corner::front_bottom_left], color);
    add_line(corners[(int)aabb::corner::front_top_right], corners[(int)aabb::corner::front_bottom_right], color);
}

void render_system_debug::add_obb(obb bounds, color color)
{
    vector3 corners[obb::k_corner_count];
    bounds.get_corners(corners);

    add_line(corners[(int)obb::corner::back_top_left], corners[(int)obb::corner::back_top_right], color);
    add_line(corners[(int)obb::corner::front_top_left], corners[(int)obb::corner::front_top_right], color);
    add_line(corners[(int)obb::corner::back_top_left], corners[(int)obb::corner::front_top_left], color);
    add_line(corners[(int)obb::corner::back_top_right], corners[(int)obb::corner::front_top_right], color);
    add_line(corners[(int)obb::corner::back_bottom_left], corners[(int)obb::corner::back_bottom_right], color);
    add_line(corners[(int)obb::corner::front_bottom_left], corners[(int)obb::corner::front_bottom_right], color);
    add_line(corners[(int)obb::corner::back_bottom_left], corners[(int)obb::corner::front_bottom_left], color);
    add_line(corners[(int)obb::corner::back_bottom_right], corners[(int)obb::corner::front_bottom_right], color);
    add_line(corners[(int)obb::corner::back_top_left], corners[(int)obb::corner::back_bottom_left], color);
    add_line(corners[(int)obb::corner::back_top_right], corners[(int)obb::corner::back_bottom_right], color);
    add_line(corners[(int)obb::corner::front_top_left], corners[(int)obb::corner::front_bottom_left], color);
    add_line(corners[(int)obb::corner::front_top_right], corners[(int)obb::corner::front_bottom_right], color);
}

void render_system_debug::add_sphere(sphere bounds, color color)
{
    constexpr int k_tesselation = 15;

    float radius = bounds.radius;
    vector3 origin = bounds.origin;

    const size_t k_vertical_segments = k_tesselation;
    const size_t k_horizontal_segments = k_tesselation * 2;

    std::vector<vector3> positions;
    positions.reserve(k_vertical_segments * k_horizontal_segments);

    // Construct vertex list for the vertical and horizontal bands on the sphere.
    for (size_t i = 0; i <= k_vertical_segments; i++)
    {
        const float latitude = (static_cast<float>(i) * math::pi / static_cast<float>(k_vertical_segments)) - math::halfpi;

        float dy = sin(latitude);
        float dxz = cos(latitude);

        for (size_t j = 0; j <= k_horizontal_segments; j++)
        {
            const float longitude = static_cast<float>(i) * math::pi2 / static_cast<float>(k_horizontal_segments);

            float dx = sin(longitude);
            float dz = cos(longitude);

            dx *= dxz;
            dz *= dxz;

            vector3 normal(dx, dy, dz);
            positions.push_back(origin + (normal * radius));
        }
    }

    // Generate the line list.
    const size_t k_stride = k_horizontal_segments + 1;

    for (size_t i = 0; i < k_vertical_segments; i++)
    {
        for (size_t j = 0; j <= k_horizontal_segments; j++)
        {
            const size_t next_i = i + 1;
            const size_t next_j = (j + 1) % k_stride;

            const size_t i0 = i * k_stride + j;
            const size_t i1 = next_i * k_stride + j;
            const size_t i2 = i * k_stride + next_j;

            const size_t i3 = i * k_stride + next_j;
            const size_t i4 = next_i * k_stride + j;
            const size_t i5 = next_i * k_stride + next_j;

            add_triangle(positions[i0], positions[i1], positions[i2], color);
            add_triangle(positions[i3], positions[i4], positions[i5], color);
        }
    }
}

void render_system_debug::add_frustum(frustum bounds, color color)
{
    vector3 corners[frustum::k_corner_count];
    bounds.get_corners(corners);

    add_line(corners[(int)frustum::corner::far_top_left], corners[(int)frustum::corner::far_top_right], color);
    add_line(corners[(int)frustum::corner::far_bottom_left], corners[(int)frustum::corner::far_bottom_right], color);
    add_line(corners[(int)frustum::corner::far_top_left], corners[(int)frustum::corner::far_bottom_left], color);
    add_line(corners[(int)frustum::corner::far_top_right], corners[(int)frustum::corner::far_bottom_right], color);

    add_line(corners[(int)frustum::corner::near_top_left], corners[(int)frustum::corner::near_top_right], color);
    add_line(corners[(int)frustum::corner::near_top_left], corners[(int)frustum::corner::near_bottom_right], color);
    add_line(corners[(int)frustum::corner::near_bottom_left], corners[(int)frustum::corner::near_bottom_left], color);
    add_line(corners[(int)frustum::corner::near_top_right], corners[(int)frustum::corner::near_bottom_right], color);

    add_line(corners[(int)frustum::corner::near_top_left], corners[(int)frustum::corner::far_top_left], color);
    add_line(corners[(int)frustum::corner::near_top_right], corners[(int)frustum::corner::far_top_right], color);
    add_line(corners[(int)frustum::corner::near_bottom_left], corners[(int)frustum::corner::far_bottom_left], color);
    add_line(corners[(int)frustum::corner::near_bottom_right], corners[(int)frustum::corner::far_bottom_right], color);
}

void render_system_debug::add_triangle(vector3 a, vector3 b, vector3 c, color color)
{
    add_line(a, b, color);
    add_line(b, c, color);
    add_line(c, a, color);
}

}; // namespace ws
