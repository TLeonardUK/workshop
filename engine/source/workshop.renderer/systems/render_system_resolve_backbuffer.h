// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"

namespace ws {

class renderer;

// ================================================================================================
//  Resolves the gbuffer to the final image on the backbuffer.
// ================================================================================================
class render_system_resolve_backbuffer
    : public render_system
{
public:
    render_system_resolve_backbuffer(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void create_graph(render_graph& graph) override;
    virtual void step(const render_world_state& state) override;

private:
    render_pass_fullscreen* m_render_pass;
    std::unique_ptr<ri_param_block> m_resolve_param_block;

};

}; // namespace ws
