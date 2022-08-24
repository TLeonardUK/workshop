// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"

namespace ws {

class renderer;

// ================================================================================================
//  Copies the final output to the backbuffer target.
// ================================================================================================
class render_system_copy_to_backbuffer
    : public render_system
{
public:
    render_system_copy_to_backbuffer(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void create_graph(render_graph& graph) override;
    virtual void step(const render_world_state& state) override;

private:
    renderer& m_renderer;

};

}; // namespace ws
