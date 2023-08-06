// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_param_block.h"

namespace ws {

class renderer;

// ================================================================================================
//  Renders the scenes transparent geometry to the resolved lighting buffer.
// ================================================================================================
class render_system_transparent_geometry
    : public render_system
{
public:
    render_system_transparent_geometry(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;
    virtual void step(const render_world_state& state) override;

private:
    result<void> create_resources();
    result<void> destroy_resources();

private:
    std::unique_ptr<ri_texture> m_accumulation_buffer;
    std::unique_ptr<ri_texture> m_revealance_buffer;
    std::unique_ptr<ri_param_block> m_resolve_param_block;

};

}; // namespace ws
