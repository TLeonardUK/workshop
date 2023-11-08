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
    render_view* get_main_view();

    result<void> create_resources();
    result<void> destroy_resources();

private:

    size_t m_probe_ray_count = 192;
    size_t m_probe_regenerations_per_frame = 1;
    float m_probe_far_z = 10000.0f;

    struct dirty_probe
    {
        render_light_probe_grid::probe* probe;
        render_light_probe_grid* grid;
        float distance;
    };


    std::unique_ptr<ri_buffer> m_scratch_buffer;
    std::unique_ptr<ri_param_block> m_regeneration_param_block;
    std::vector<std::unique_ptr<ri_param_block>> m_probe_param_block;

    std::unique_ptr<render_batch_instance_buffer> m_regeneration_instance_buffer;

    std::vector<dirty_probe> m_dirty_probes;
    std::vector<dirty_probe> m_probes_to_regenerate;
    vector3 m_last_dirty_view_position;

    std::unique_ptr<ri_query> m_gpu_time_query;
    double m_gpu_time = 0.0f;
    size_t m_adjusted_max_probes_per_frame = 1;
    double m_average_gpu_time = 0.0f;
    size_t m_probes_rengerated_last_frame = 0;

    bool m_should_regenerate = false;

};

}; // namespace ws
