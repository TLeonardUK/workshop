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
#include "workshop.renderer/passes/render_pass_primitives.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"

#include "workshop.core/math/matrix4.h"
#include "workshop.core/perf/profile.h"

#include "workshop.render_interface/ri_interface.h"

namespace {

constexpr int k_tesselation = 11;
constexpr float k_float_tesselation = static_cast<float>(k_tesselation);

};

namespace ws {

render_system_debug::render_system_debug(renderer& render)
    : render_system(render, "debug")
{
}

void render_system_debug::register_init(init_list& list)
{
}

void render_system_debug::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal) ||
         view.has_flag(render_view_flags::scene_only))
    {
        return;
    }

    if (m_vertex_buffer == nullptr)
    {
        return;
    }

    std::unique_ptr<render_pass_primitives> pass = std::make_unique<render_pass_primitives>();
    pass->name = "debug primitives";
    pass->system = this;
    pass->technique = m_renderer.get_effect_manager().get_technique("render_debug_primitive", {});
    pass->output.color_targets = m_renderer.get_swapchain_output().color_targets;
    pass->output.depth_target = m_renderer.get_gbuffer_output().depth_target;
    pass->vertex_buffer = m_vertex_buffer.get();
    pass->index_buffer = m_index_buffer.get();
    pass->vertex_count = m_draw_vertex_count;
    graph.add_node(std::move(pass));
}

void render_system_debug::step(const render_world_state& state)
{
    std::scoped_lock mutex(m_vertices_mutex);

    ri_interface& ri = m_renderer.get_render_interface();

    if (m_queued_vertex_count == 0)
    {
        m_draw_vertex_count = 0;
        return;
    }

    // Make sure vertex buffer has enough space.
    if (m_vertex_buffer == nullptr || m_vertex_buffer->get_element_count() < m_queued_vertex_count)
    {
        ri_buffer::create_params params;
        params.element_count = m_queued_vertex_count;
        params.element_size = sizeof(debug_primitive_vertex);
        params.usage = ri_buffer_usage::vertex_buffer;

        m_vertex_buffer = ri.create_buffer(params, "Debug Primitive Vertex Buffer");
    }

    // Make sure index buffer has enough space.
    if (m_index_buffer == nullptr || m_index_buffer->get_element_count() < m_queued_vertex_count)
    {
        ri_buffer::create_params params;
        params.element_count = m_queued_vertex_count;
        params.element_size = sizeof(uint32_t);
        params.usage = ri_buffer_usage::index_buffer;

        m_index_buffer = ri.create_buffer(params, "Debug Primitive Index Buffer");

        // Update index buffer.
        uint32_t* index_ptr = reinterpret_cast<uint32_t*>(m_index_buffer->map(0, m_queued_vertex_count * sizeof(uint32_t)));
        for (size_t i = 0; i < m_queued_vertex_count; i++)
        {
            index_ptr[i] = static_cast<uint32_t>(i);
        }
        m_index_buffer->unmap(index_ptr);
    }

    // Update vertex buffer.
    debug_primitive_vertex* vertex_ptr = reinterpret_cast<debug_primitive_vertex*>(m_vertex_buffer->map(0, m_queued_vertex_count * sizeof(debug_primitive_vertex)));
    memcpy(vertex_ptr, m_vertices.data(), m_queued_vertex_count * sizeof(debug_primitive_vertex));
    m_vertex_buffer->unmap(vertex_ptr);    

    // Reset vertex buffer so we can start filling it again.
    m_draw_vertex_count = m_queued_vertex_count;
    m_queued_vertex_count = 0;
}

render_system_debug::shape& render_system_debug::find_or_create_cached_shape(shape_type type)
{
    std::scoped_lock mutex(m_cached_shape_mutex);

    for (auto& s : m_cached_shapes)
    {
        if (s->type == type)
        {
            return *s;
        }
    }

    std::unique_ptr<shape> new_shape = std::make_unique<shape>();
    new_shape->type = type;

    shape* result = new_shape.get();
    m_cached_shapes.push_back(std::move(new_shape));
    return *result;
}

void render_system_debug::add_cached_shape(const shape& shape, const vector3& offset, const vector3& scale, const color& color)
{
    std::scoped_lock mutex(m_vertices_mutex);

    size_t start_index = m_queued_vertex_count;
    m_queued_vertex_count += shape.positions.size();
    if (m_queued_vertex_count > m_vertices.size())
    {
        m_vertices.resize(m_queued_vertex_count);
    }

    for (size_t i = 0; i < shape.positions.size(); i += 2)
    {
        vector3 p1 = shape.positions[i];
        vector3 p2 = shape.positions[i + 1];

        p1 = offset + (p1 * scale);
        p2 = offset + (p2 * scale);

        debug_primitive_vertex& v0 = m_vertices[start_index + i + 0];
        v0.position = p1;
        v0.color = color.rgba();

        debug_primitive_vertex& v1 = m_vertices[start_index + i + 1];
        v1.position = p2;
        v1.color = color.rgba();
    }
}

void render_system_debug::add_line(const vector3& start, const vector3& end, const color& color)
{
    std::scoped_lock mutex(m_vertices_mutex);

    size_t start_index = m_queued_vertex_count;
    m_queued_vertex_count += 2;
    if (m_queued_vertex_count > m_vertices.size())
    {
        m_vertices.resize(m_queued_vertex_count);
    }

    debug_primitive_vertex& v0 = m_vertices[start_index + 0];
    v0.position = start;
    v0.color = color.rgba();

    debug_primitive_vertex& v1 = m_vertices[start_index + 1];
    v1.position = end;
    v1.color = color.rgba();
}

void render_system_debug::add_aabb(const aabb& bounds, const color& color)
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

void render_system_debug::add_obb(const obb& bounds, const color& color)
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

void render_system_debug::add_sphere(const sphere& bounds, const color& color)
{
    shape& cached_shape = find_or_create_cached_shape(shape_type::sphere);
    if (cached_shape.positions.empty())
    {
        // Make horizontal bands going upwards.
        for (size_t i = 0; i <= k_tesselation; i++)
        {
            float vertical_delta = (i / k_float_tesselation);
            float plane_radius = sin(vertical_delta * math::pi);
            float y = cos(vertical_delta * math::pi);

            for (size_t j = 0; j <= k_tesselation; j++)
            {
                float radial_delta = (j / k_float_tesselation);
                float next_radial_delta = ((j + 1) % (k_tesselation + 1)) / k_float_tesselation;

                float x = sin(radial_delta * math::pi2) * plane_radius;
                float z = cos(radial_delta * math::pi2) * plane_radius;
                float next_x = sin(next_radial_delta * math::pi2) * plane_radius;
                float next_z = cos(next_radial_delta * math::pi2) * plane_radius;

                cached_shape.positions.push_back(vector3(x, y, z));
                cached_shape.positions.push_back(vector3(next_x, y, next_z));
            }            
        }

        // Makes vertical bands going around the sphere.
        for (size_t i = 0; i <= k_tesselation; i++)
        {
            float horizontal_delta = (i / k_float_tesselation);
            float angle = horizontal_delta * math::pi2;

            for (size_t j = 0; j < k_tesselation; j++)
            {
                float vertical_delta = (j / k_float_tesselation);
                float next_vertical_delta = ((j + 1) % (k_tesselation + 1)) / k_float_tesselation;

                float plane_radius = sin(vertical_delta * math::pi);
                float next_plane_radius = sin(next_vertical_delta * math::pi);

                float y = cos(vertical_delta * math::pi);
                float next_y = cos(next_vertical_delta * math::pi);

                float radial_delta = sin(vertical_delta * math::pi2) * plane_radius;
                float nex_radial_delta = sin(next_vertical_delta * math::pi2) * next_plane_radius;

                float x = sin(angle) * plane_radius;
                float next_x = sin(angle) * next_plane_radius;

                float z = cos(angle) * plane_radius;
                float next_z = cos(angle) * next_plane_radius;

                cached_shape.positions.push_back(vector3(x, y, z));
                cached_shape.positions.push_back(vector3(next_x, next_y, next_z));
            }
        }
    }

    // Transform cached shape and add to screen.
    add_cached_shape(cached_shape, bounds.origin, vector3(bounds.radius, bounds.radius, bounds.radius), color);
}

void render_system_debug::add_frustum(const frustum& bounds, const color& color)
{
    vector3 corners[frustum::k_corner_count];
    bounds.get_corners(corners);

    add_line(corners[(int)frustum::corner::far_top_left], corners[(int)frustum::corner::far_top_right], color);
    add_line(corners[(int)frustum::corner::far_bottom_left], corners[(int)frustum::corner::far_bottom_right], color);
    add_line(corners[(int)frustum::corner::far_top_left], corners[(int)frustum::corner::far_bottom_left], color);
    add_line(corners[(int)frustum::corner::far_top_right], corners[(int)frustum::corner::far_bottom_right], color);

    add_line(corners[(int)frustum::corner::near_top_left], corners[(int)frustum::corner::near_top_right], color);
    add_line(corners[(int)frustum::corner::near_bottom_left], corners[(int)frustum::corner::near_bottom_right], color);
    add_line(corners[(int)frustum::corner::near_top_left], corners[(int)frustum::corner::near_bottom_left], color);
    add_line(corners[(int)frustum::corner::near_top_right], corners[(int)frustum::corner::near_bottom_right], color);

    add_line(corners[(int)frustum::corner::near_top_left], corners[(int)frustum::corner::far_top_left], color);
    add_line(corners[(int)frustum::corner::near_top_right], corners[(int)frustum::corner::far_top_right], color);
    add_line(corners[(int)frustum::corner::near_bottom_left], corners[(int)frustum::corner::far_bottom_left], color);
    add_line(corners[(int)frustum::corner::near_bottom_right], corners[(int)frustum::corner::far_bottom_right], color);
}

void render_system_debug::add_triangle(const vector3& a, const vector3& b, const vector3& c, const color& color)
{
    add_line(a, b, color);
    add_line(b, c, color);
    add_line(c, a, color);
}

void render_system_debug::add_cylinder(const cylinder& bounds, const color& color)
{
    float top_y = (bounds.height * 0.5f);
    float bottom_y = -(bounds.height * 0.5f);

    matrix4 transform = bounds.get_transform();

    for (size_t j = 0; j <= k_tesselation; j++)
    {
        float radial_delta = (j / k_float_tesselation);
        float next_radial_delta = ((j + 1) % (k_tesselation + 1)) / k_float_tesselation;

        float x = sin(radial_delta * math::pi2) * bounds.radius;
        float z = cos(radial_delta * math::pi2) * bounds.radius;
        float next_x = sin(next_radial_delta * math::pi2) * bounds.radius;
        float next_z = cos(next_radial_delta * math::pi2) * bounds.radius;

        vector3 top_vertex = vector3(x, top_y, z) * transform;
        vector3 bottom_vertex = vector3(x, bottom_y, z) * transform;
        vector3 next_top_vertex = vector3(next_x, top_y, next_z) * transform;
        vector3 next_bottom_vertex = vector3(next_x, bottom_y, next_z) * transform;

        add_line(top_vertex, next_top_vertex, color);
        add_line(bottom_vertex, next_bottom_vertex, color);
        add_line(bottom_vertex, top_vertex, color);
    }
}

void render_system_debug::add_capsule(const cylinder& bounds, const color& color)
{
    float cap_radius = bounds.radius;
    cylinder body_bounds(bounds.origin, bounds.orientation, bounds.radius, bounds.height - (cap_radius * 2.0f));

    matrix4 transform = bounds.get_transform();

    vector3 bottom_center = vector3(0.0f, -bounds.height * 0.5f, 0.0f) * transform;
    vector3 top_center = vector3(0.0f, bounds.height * 0.5f, 0.0f) * transform;

    // The body of the capsule.
    add_cylinder(bounds, color);

    // Add hemispheres to top and bottom
    add_hemisphere(hemisphere(top_center, bounds.orientation, bounds.radius), color, false);
    add_hemisphere(hemisphere(bottom_center, bounds.orientation * quat::angle_axis(math::pi, vector3::forward), bounds.radius), color, false);
}

void render_system_debug::add_hemisphere(const hemisphere& bounds, const color& color, bool horizontal_bands)
{
    float radius = bounds.radius;
    matrix4 transform = bounds.get_transform();

    // Make horizontal bands going upwards.
    if (horizontal_bands)
    {
        for (size_t i = 0; i <= k_tesselation; i++)
        {
            float vertical_delta = (i / k_float_tesselation);
            float plane_radius = sin(vertical_delta * math::halfpi) * radius;
            float y = cos(vertical_delta * math::halfpi) * radius;

            for (size_t j = 0; j <= k_tesselation; j++)
            {
                float radial_delta = (j / k_float_tesselation);
                float next_radial_delta = ((j + 1) % (k_tesselation + 1)) / k_float_tesselation;

                float x = sin(radial_delta * math::pi2) * plane_radius;
                float z = cos(radial_delta * math::pi2) * plane_radius;
                float next_x = sin(next_radial_delta * math::pi2) * plane_radius;
                float next_z = cos(next_radial_delta * math::pi2) * plane_radius;

                vector3 vert = vector3(x, y, z) * transform;
                vector3 next_vert = vector3(next_x, y, next_z) * transform;

                add_line(vert, next_vert, color);
            }
        }
    }

    // Makes vertical bands going around the sphere.
    for (size_t i = 0; i <= k_tesselation; i++)
    {
        float horizontal_delta = (i / k_float_tesselation);
        float angle = horizontal_delta * math::pi2;

        for (size_t j = 0; j < k_tesselation; j++)
        {
            float vertical_delta = (j / k_float_tesselation);
            float next_vertical_delta = ((j + 1) % (k_tesselation + 1)) / k_float_tesselation;

            float plane_radius = sin(vertical_delta * math::halfpi) * radius;
            float next_plane_radius = sin(next_vertical_delta * math::halfpi) * radius;

            float y = cos(vertical_delta * math::halfpi) * radius;
            float next_y = cos(next_vertical_delta * math::halfpi) * radius;

            float radial_delta = sin(vertical_delta * math::pi2) * plane_radius;
            float nex_radial_delta = sin(next_vertical_delta * math::pi2) * next_plane_radius;

            float x = sin(angle) * plane_radius;
            float next_x = sin(angle) * next_plane_radius;

            float z = cos(angle) * plane_radius;
            float next_z = cos(angle) * next_plane_radius;

            vector3 vert = vector3(x, y, z) * transform;
            vector3 next_vert = vector3(next_x, next_y, next_z) * transform;

            add_line(vert, next_vert, color);
        }
    }
}

void render_system_debug::add_cone(const vector3& start, const vector3& end, float radius, const color& color)
{
    vector3 origin = start;
    vector3 normal = (end - start).normalize();

    float height = (end - start).length();

    quat rotation = quat::rotate_to(vector3::up, normal); 

    matrix4 transform = matrix4::rotation(rotation) * matrix4::translate(origin);

    for (size_t i = 0; i <= k_tesselation; i++)
    {
        float horizontal_delta = (i / k_float_tesselation);
        float angle = horizontal_delta * math::pi2;

        float next_horizontal_delta = ((i + 1) % (k_tesselation + 1)) / k_float_tesselation;
        float next_angle = next_horizontal_delta * math::pi2;

        vector3 vert = vector3(sin(angle) * radius, 0.0f, cos(angle) * radius) * transform;
        vector3 next_vert = vector3(sin(next_angle) * radius, 0.0f, cos(next_angle) * radius) * transform;
        vector3 top_vert = vector3(0.0f, height, 0.0f) * transform;

        add_line(vert, next_vert, color);
        add_line(vert, top_vert, color);
    }
}

void render_system_debug::add_arrow(const vector3& start, const vector3& end, const color& color)
{
    float total_length = (end - start).length();

    float spoke_radius = total_length * 0.05f;
    float cone_radius = spoke_radius * 3.0f;
    float cone_length = total_length * 0.3f;
    float spoke_length = (end - start).length() - cone_length;

    vector3 normal = (end - start).normalize();
    vector3 spoke_center = start + (normal * (spoke_length * 0.5f));

    add_cylinder(cylinder(spoke_center, quat::rotate_to(vector3::up, normal), spoke_radius, spoke_length), color);
    add_cone(start + (normal * spoke_length), end, cone_radius, color);
}

void render_system_debug::add_truncated_cone(const vector3& start, const vector3& end, float start_radius, float end_radius, const color& color)
{
    vector3 origin = start;
    vector3 normal = (end - start).normalize();

    float height = (end - start).length();

    quat rotation = quat::rotate_to(vector3::up, normal);

    matrix4 transform = matrix4::translate(origin) * matrix4::rotation(rotation);

    for (size_t i = 0; i <= k_tesselation; i++)
    {
        float horizontal_delta = (i / k_float_tesselation);
        float angle = horizontal_delta * math::pi2;

        float next_horizontal_delta = ((i + 1) % (k_tesselation + 1)) / k_float_tesselation;
        float next_angle = next_horizontal_delta * math::pi2;

        vector3 vert = vector3(sin(angle) * start_radius, 0.0f, cos(angle) * start_radius) * transform;
        vector3 next_vert = vector3(sin(next_angle) * start_radius, 0.0f, cos(next_angle) * start_radius) * transform;
        vector3 top_vert = vector3(sin(angle) * end_radius, height, cos(angle) * end_radius) * transform;
        vector3 next_top_vert = vector3(sin(next_angle) * end_radius, height, cos(next_angle) * end_radius) * transform;

        add_line(vert, next_vert, color);
        add_line(top_vert, next_top_vert, color);
        add_line(vert, top_vert, color);
    }
}

}; // namespace ws
