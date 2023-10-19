// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"
#include "workshop.renderer/render_pass.h"

#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_buffer.h"

#include "workshop.core/math/vector2.h"
#include "workshop.core/math/vector4.h"
#include "workshop.core/math/rect.h"
#include "workshop.core/math/frustum.h"
#include "workshop.core/math/sphere.h"
#include "workshop.core/math/hemisphere.h"
#include "workshop.core/math/cylinder.h"

namespace ws {

class renderer;
class render_pass_callback;
class render_view;

// ================================================================================================
//  Renders the debug primitives
// ================================================================================================
class render_system_debug
    : public render_system
{
public:
    render_system_debug(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void step(const render_world_state& state) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;

    void add_line(const vector3& start, const vector3& end, const color& color);
    void add_aabb(const aabb& bounds, const color& color);
    void add_obb(const obb& bounds, const color& color);
    void add_sphere(const sphere& bounds, const color& color);
    void add_frustum(const frustum& bounds, const color& color);
    void add_triangle(const vector3& a, const vector3& b, const vector3& c, const color& color);
    void add_cylinder(const cylinder& bounds, const color& color);
    void add_capsule(const cylinder& bounds, const color& color);
    void add_hemisphere(const hemisphere& bounds, const color& color, bool horizontal_bands = true);
    void add_cone(const vector3& origin, const vector3& end, float radius, const color& color);
    void add_arrow(const vector3& start, const vector3& end, const color& color);
    void add_truncated_cone(const vector3& start, const vector3& end, float start_radius, float end_radius, const color& color);

private:

    struct debug_primitive_vertex
    {
        vector3 position;
        vector4 color;
    };

    // Used to store a precalculated shape to avoid recalculating
    // positional information each time a debug element is added.
    enum class shape_type
    {
        sphere,
    };

    struct shape
    {
        shape_type type;
        std::vector<vector3> positions;
    };

private:

    shape& find_or_create_cached_shape(shape_type type);
    void add_cached_shape(const shape& shape, const vector3& offset, const vector3& scale, const color& color);

private:

    std::mutex m_cached_shape_mutex;
    std::vector<std::unique_ptr<shape>> m_cached_shapes;

    std::mutex m_vertices_mutex;
    std::vector<debug_primitive_vertex> m_vertices;
    size_t m_queued_vertex_count = 0;
    size_t m_draw_vertex_count = 0;

    std::unique_ptr<ri_buffer> m_position_buffer;
    std::unique_ptr<ri_buffer> m_color0_buffer;
    std::unique_ptr<ri_buffer> m_index_buffer;

};

}; // namespace ws
