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
//  Raytraces the scene to an output target. This is primarily used for debugging the raytracing
//  shaders, its not directly used during normal gameplay.
// ================================================================================================
class render_system_raytrace_scene
    : public render_system
{
public:
    render_system_raytrace_scene(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;
    virtual void step(const render_world_state& state) override;
    virtual void swapchain_resized() override;

    ri_texture& get_output_buffer();

private:
    result<void> create_resources();
    result<void> destroy_resources();

private:
    std::unique_ptr<ri_texture> m_scene_texture;
    
};

}; // namespace ws
