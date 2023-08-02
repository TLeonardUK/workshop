// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_system.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/objects/render_reflection_probe.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_buffer.h"

namespace ws {

class renderer;

// ================================================================================================
//  Responsible for regenerating reflection probes.
// ================================================================================================
class render_system_reflection_probes
    : public render_system
{
public:
    inline static constexpr size_t k_probe_cubemap_size = 512;
    inline static constexpr size_t k_probe_cubemap_mips = 10;
    inline static constexpr size_t k_probe_regenerations_per_frame = 1;
    inline static constexpr float k_probe_near_z = 10.0f;
    inline static constexpr float k_probe_far_z = 10000.0f;

public:
    render_system_reflection_probes(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;
    virtual void build_post_graph(render_graph& graph, const render_world_state& state) override;
    virtual void step(const render_world_state& state) override;

private:
    void regenerate_probe(render_reflection_probe* probe, size_t regeneration_index);

    result<void> create_resources();
    result<void> destroy_resources();

private:

    struct view_info
    {
        render_object_id id;
        render_reflection_probe* probe;
        ri_texture* render_target;
    };

    std::vector<std::unique_ptr<ri_texture>> m_probe_capture_targets;
    std::vector<std::unique_ptr<ri_param_block>> m_convolve_param_blocks;
    std::vector<view_info> m_probe_capture_views;

    size_t m_probes_regenerating;

};

}; // namespace ws
