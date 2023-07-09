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

    // Identifier used to reference a registered texture in a imgui draw command.
    using texture_id = size_t;

    // Should match imgui_vertex in shader.
#pragma pack(push, 1)
    struct vertex
    {
        vector2 position;
        vector2 uv;
        vector4 color;
    };
#pragma pack(pop)

    struct draw_command
    {
        texture_id texture;

        vector2 display_size;
        vector2 display_pos;

        rect scissor;
        size_t offset;
        size_t count;
    };

public:
    render_system_imgui(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void create_graph(render_graph& graph) override;
    virtual void step(const render_world_state& state) override;

    // Registers a texture that can be used in imgui rendering.
    texture_id register_texture(std::unique_ptr<ri_texture> texture, bool default_texture);

    // Unregisters a texture previously registered for imgui rendering.
    void unregister_texture(texture_id id);

    // Updates the render data we will use to draw to screen.
    void update_draw_data(const std::vector<draw_command>& commands, const std::vector<vertex>& vertices, const std::vector<uint16_t>& indices);

private:

    void generate(renderer& renderer, render_pass::generated_state& output, render_view& view);

private:
    
    std::unordered_map<texture_id, std::unique_ptr<ri_texture>> m_textures;
    texture_id m_default_texture;
    texture_id m_next_texture_id = 1;

    std::vector<draw_command> m_draw_commands;
    std::vector<vertex> m_draw_vertices;
    std::vector<uint16_t> m_draw_indices;

    std::unique_ptr<ri_buffer> m_vertex_buffer;
    std::unique_ptr<ri_buffer> m_index_buffer;

    render_pass_callback* m_render_pass;

};

}; // namespace ws
