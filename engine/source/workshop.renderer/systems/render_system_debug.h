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
    virtual void create_graph(render_graph& graph) override;
    virtual void step(const render_world_state& state) override;

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

    void generate(renderer& renderer, render_pass::generated_state& output, render_view& view);

private:

#pragma pack(push, 1)
    struct debug_primitive_vertex
    {
        vector3 position;
        vector4 color;
    };
#pragma pack(pop)

    std::vector<debug_primitive_vertex> m_vertices;
    size_t m_draw_vertex_count = 0;

    std::unique_ptr<ri_buffer> m_vertex_buffer;
    std::unique_ptr<ri_buffer> m_index_buffer;

    render_pass_callback* m_render_pass;

};

}; // namespace ws
