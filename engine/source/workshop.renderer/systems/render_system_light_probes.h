// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/objects/render_light_probe_grid.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_buffer.h"

namespace ws {

class renderer;
class render_light_probe_grid;

// ================================================================================================
//  Responsible for regenerating diffuse light probes.
// ================================================================================================
class render_system_light_probes
    : public render_system
{
public:
    render_system_light_probes(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;
    virtual void build_post_graph(render_graph& graph, const render_world_state& state) override;
    virtual void step(const render_world_state& state) override;

    bool is_regenerating();

    void regenerate();

private:
    void regenerate_probe(render_light_probe_grid* grid, render_light_probe_grid::probe* probe, size_t regeneration_index);

    result<void> create_resources();
    result<void> destroy_resources();

private:

    size_t m_probe_cubemap_size = 128;
    size_t m_probe_regenerations_per_frame = 1;
    float m_probe_near_z = 10.0f;
    float m_probe_far_z = 10000.0f;

    struct view_info
    {
        render_object_id id;
        render_light_probe_grid* grid;
        size_t probe_index;
        ri_texture* render_target;
        std::unique_ptr<ri_param_block> resolve_params;
    };

    struct dirty_probe
    {
        render_light_probe_grid::probe* probe;
        render_light_probe_grid* grid;
        float distance;
    };

    std::vector<dirty_probe> m_dirty_probes;
    vector3 m_last_dirty_view_position;

    std::vector<std::unique_ptr<ri_texture>> m_probe_capture_targets;
    std::vector<view_info> m_probe_capture_views;

    size_t m_probes_regenerating;
    bool m_should_regenerate = false;

};

}; // namespace ws
