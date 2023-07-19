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

// ================================================================================================
//  Responsible for generating shadow maps for lights which are then used in the lighting
//  render system.
// ================================================================================================
class render_system_shadows
    : public render_system
{
public:
    struct cascade_info
    {
        render_object_id view_id = 0;
        std::unique_ptr<ri_texture> shadow_map;
        ri_texture_view shadow_map_view;
        size_t map_size = 0;

        float z_near = 0.0f;
        float z_far = 0.0f;

        float blend_factor = 0.0f;

        float split_min_distance;
        float split_max_distance;

        float world_radius;

        matrix4 projection_matrix;
        matrix4 view_matrix;
        frustum view_frustum;
        frustum frustum;

        bool needs_render = true;
        size_t last_rendered_frame = 0;

        std::unique_ptr<ri_param_block> shadow_map_state_param_block;
    };

    struct shadow_info
    {
        render_object_id light_id;
        render_object_id view_id;

        frustum view_frustum;
        frustum view_view_frustum;
        quat light_rotation;
        vector3 light_location;

        std::vector<cascade_info> cascades;
    };

    static inline constexpr size_t k_max_cascades_updates_per_frame = 1;

    shadow_info& find_or_create_shadow_info(render_object_id light_id, render_object_id view_id);

public:
    render_system_shadows(renderer& render);

    virtual void register_init(init_list& list) override;
    virtual void build_graph(render_graph& graph, const render_world_state& state, render_view& view) override;
    virtual void step(const render_world_state& state) override;

private:
    result<void> create_resources();
    result<void> destroy_resources();

    void step_directional_shadow(render_view* view, render_directional_light* light);
    void step_point_shadow(render_view* view, render_point_light* light);
    void step_spot_shadow(render_view* view, render_spot_light* light);

    void destroy_cascade(cascade_info& info);
    void step_cascade(shadow_info& info, cascade_info& shadow_info);

    void update_cascade_param_block(cascade_info& cascade);

private:
    std::vector<shadow_info> m_shadow_info;

};

}; // namespace ws
