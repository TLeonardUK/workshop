// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_buffer.h"

namespace ws {

class renderer;

// Should match the values in the lighting shader.
enum class render_light_type
{
    directional = 0,
    point = 1,
    spotlight = 2,
};

// ================================================================================================
//  Responsible for clustering, accumulating and applying lighting.
// ================================================================================================
class render_system_lighting
    : public render_system
{
public:
    render_system_lighting(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;
    virtual void step(const render_world_state& state) override;

    ri_texture& get_lighting_buffer();

private:
    result<void> create_resources();
    result<void> destroy_resources();

private:

    std::unique_ptr<ri_texture> m_lighting_buffer;
    render_output m_lighting_output;

    // Maximum number of lights we can handle on screen at once.
    static inline constexpr size_t k_max_lights = 10000;

};

}; // namespace ws
