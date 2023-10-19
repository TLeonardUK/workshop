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

#include "thirdparty/imgui/imgui.h"

namespace ws {

class renderer;
class render_pass_callback;
class render_view;

// ================================================================================================
//  Renders the imgui context to the composited RT.
// ================================================================================================
class render_system_imgui
    : public render_system
{
public:

    struct vertex
    {
        vector2 position;
        vector2 uv;
        vector4 color;
    };

    struct draw_command
    {
        ImTextureID texture;

        vector2 display_size;
        vector2 display_pos;

        rect scissor;
        size_t offset;
        size_t count;
    };

public:
    render_system_imgui(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;
    virtual void step(const render_world_state& state) override;

    // Sets the default imgui texture.
    void set_default_texture(std::unique_ptr<ri_texture> texture);

    // Updates the render data we will use to draw to screen.
    void update_draw_data(const std::vector<draw_command>& commands, const std::vector<vertex>& vertices, const std::vector<uint16_t>& indices);

private:
    
    std::unique_ptr<ri_texture> m_default_texture;
    
    std::vector<draw_command> m_draw_commands;
    std::vector<vertex> m_draw_vertices;
    std::vector<uint16_t> m_draw_indices;

    std::unique_ptr<ri_buffer> m_position_buffer;
    std::unique_ptr<ri_buffer> m_uv0_buffer;
    std::unique_ptr<ri_buffer> m_color0_buffer;
    std::unique_ptr<ri_buffer> m_index_buffer;

};

}; // namespace ws
