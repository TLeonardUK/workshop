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

private:

    void generate(renderer& renderer, render_pass::generated_state& output, render_view& view);

    void add_line(vector3 start, vector3 end, color color);
    void add_aabb(aabb bounds, color color);
    void add_obb(obb bounds, color color);
    void add_sphere(sphere bounds, color color);
    void add_frustum(frustum bounds, color color);
    void add_triangle(vector3 a, vector3 b, vector3 c, color color);

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
