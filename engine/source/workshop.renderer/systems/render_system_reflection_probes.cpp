// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_reflection_probes.h"
#include "workshop.renderer/systems/render_system_light_probes.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_compute.h"
#include "workshop.renderer/passes/render_pass_calculate_mips.h"
#include "workshop.renderer/passes/render_pass_instanced_model.h"
#include "workshop.renderer/systems/render_system_debug.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/render_resource_cache.h"

namespace ws {

render_system_reflection_probes::render_system_reflection_probes(renderer& render)
    : render_system(render, "reflection probes")
{
}

void render_system_reflection_probes::register_init(init_list& list)
{
    list.add_step(
        "Reflection Probe Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> render_system_reflection_probes::create_resources()
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();

    // Create cubemap we will render the scene into.
    ri_texture::create_params params;
    params.width = k_probe_cubemap_size;
    params.height = k_probe_cubemap_size;
    params.depth = 6;
    params.mip_levels = k_probe_cubemap_mips;
    params.dimensions = ri_texture_dimension::texture_cube;
    params.is_render_target = true;
    params.format = ri_texture_format::R32G32B32A32_FLOAT;
    params.allow_unordered_access = true;
    params.allow_individual_image_access = true;

    for (size_t i = 0; i < k_probe_regenerations_per_frame; i++)
    {
        m_probe_capture_targets.push_back(m_renderer.get_render_interface().create_texture(params, "light probe capture target"));
    }

    // Create some param blocks for convolving each level of the cubemap.
    size_t param_blocks_required = k_probe_regenerations_per_frame * 6 * k_probe_cubemap_mips;
    for (size_t i = 0; i < param_blocks_required; i++)
    {
        m_convolve_param_blocks.push_back(m_renderer.get_param_block_manager().create_param_block("convolve_reflection_probe_params"));
    }

    // Create render views we will use for capturing cubemaps.
    size_t render_views_required = 6 * k_probe_regenerations_per_frame;
    m_probe_capture_views.resize(render_views_required);

    for (size_t probe = 0; probe < k_probe_regenerations_per_frame; probe++)
    {
        for (size_t face = 0; face < 6; face++)
        {
            size_t index = (probe * 6) + face;

            render_object_id view_id = m_renderer.next_render_object_id();
            scene_manager.create_view(view_id, "reflection probe capture view");

            matrix4 projection_matrix = matrix4::perspective(
                math::halfpi,
                1.0f,
                k_probe_near_z,
                k_probe_far_z
            );

            render_view* view = scene_manager.resolve_id_typed<render_view>(view_id);
            view->set_view_type(render_view_type::custom);
            view->set_view_order(render_view_order::light_probe);
            view->set_projection_matrix(projection_matrix);
            view->set_view_matrix(matrix4::identity);
            view->set_clip(k_probe_near_z, k_probe_far_z);
            view->set_should_render(false);
            view->set_flags(render_view_flags::normal | render_view_flags::scene_only | render_view_flags::constant_ambient_lighting);

            view_info info;
            info.id = view_id;
            
            m_probe_capture_views[index] = std::move(info);
        }
    }

    return true;
}

result<void> render_system_reflection_probes::destroy_resources()
{
    m_probe_capture_targets.clear();

    return true;
}

void render_system_reflection_probes::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    // Nothing to do here.
}

void render_system_reflection_probes::build_post_graph(render_graph& graph, const render_world_state& state)
{
    size_t param_block_index = 0;
    for (size_t i = 0; i < m_probes_regenerating; i++)
    {
        // Calculate the mip chain of the probe.
        std::unique_ptr<render_pass_calculate_mips> pass = std::make_unique<render_pass_calculate_mips>();
        pass->name = "calculate reflection probe mips";
        pass->system = this;
        pass->texture = m_probe_capture_targets[i].get();
        graph.add_node(std::move(pass));

        // Convolute each mip of the probe.
        for (size_t face = 0; face < 6; face++)
        {
            view_info& info = m_probe_capture_views[(i * 6) + face];

            ri_texture& texture = info.probe->get_texture();
            for (size_t mip = 0; mip < texture.get_mip_levels(); mip++)
            {
                ri_texture_view output_view;
                output_view.texture = &texture;
                output_view.slice = face;
                output_view.mip = mip;

                ri_param_block* block = m_convolve_param_blocks[param_block_index++].get();
                block->set("source_texture", *info.render_target);
                block->set("source_texture_sampler", *m_renderer.get_default_sampler(default_sampler_type::color));
                block->set("source_texture_face", (int)face);
                block->set("source_texture_size", vector2i(info.render_target->get_width(), info.render_target->get_height()));
                block->set("roughness", float(mip) / float(texture.get_mip_levels() - 1));

                std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
                pass->name = string_format("convolve reflection probe [face:%zi mip:%zi]", face, mip);
                pass->system = this;
                pass->technique = m_renderer.get_effect_manager().get_technique("convolve_reflection_probe", {});
                pass->output.color_targets.push_back(output_view);
                pass->param_blocks.push_back(block);
                graph.add_node(std::move(pass));
            }
        }
    }
}

void render_system_reflection_probes::regenerate_probe(render_reflection_probe* probe, size_t regeneration_index)
{
    ri_interface& ri_interface = m_renderer.get_render_interface();
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
 
    vector3 origin = probe->get_local_location();

    // Which direction each face of our cubemap faces.
    std::array<matrix4, 6> cascade_directions;
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::x_pos)] = matrix4::look_at(origin, origin + vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::x_neg)] = matrix4::look_at(origin, origin + vector3(-1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::y_pos)] = matrix4::look_at(origin, origin + vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, -1.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::y_neg)] = matrix4::look_at(origin, origin + vector3(0.0f, -1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::z_pos)] = matrix4::look_at(origin, origin + vector3(0.0f, 0.0f, 1.0f), vector3(0.0f, 1.0f, 0.0f));
    cascade_directions[ri_interface.get_cube_map_face_index(ri_cube_map_face::z_neg)] = matrix4::look_at(origin, origin + vector3(0.0f, 0.0f, -1.0f), vector3(0.0f, 1.0f, 0.0f));

    // Render each side of the face.
    for (size_t i = 0; i < 6; i++)
    {
        matrix4 light_view_matrix = cascade_directions[i];
        matrix4 projection_matrix = matrix4::perspective(
            math::halfpi,
            1.0f,
            k_probe_near_z,
            k_probe_far_z
        );

        size_t view_index = (regeneration_index * 6) + i;
        view_info& info = m_probe_capture_views[view_index];
        info.probe = probe;
        info.render_target = m_probe_capture_targets[regeneration_index].get();

        ri_texture_view rt_view;
        rt_view.texture = info.render_target;
        rt_view.slice = i;

        render_view* view = scene_manager.resolve_id_typed<render_view>(info.id);
        view->set_projection_matrix(projection_matrix);
        view->set_view_matrix(light_view_matrix);
        view->set_should_render(true);
        view->set_local_transform(origin, quat::identity, vector3::one);
        view->set_render_target(rt_view);
        view->set_viewport({ 0, 0, (int)info.render_target->get_width(), (int)info.render_target->get_height() });
    }
    
    // Mark probe as not dirty anymore.
    probe->mark_regenerated();
}

void render_system_reflection_probes::step(const render_world_state& state)
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    render_system_debug* debug_system = m_renderer.get_system<render_system_debug>();
    render_system_light_probes* light_probe_system = m_renderer.get_system<render_system_light_probes>();

    std::vector<render_reflection_probe*> reflection_probes = scene_manager.get_reflection_probes();

    m_probes_regenerating = 0;

    // Disable all our views from rendering.
    for (size_t i = 0; i < m_probe_capture_views.size(); i++)
    {
        render_view* view = scene_manager.resolve_id_typed<render_view>(m_probe_capture_views[i].id);
        view->set_should_render(false);
    }

    // Do not try and regenerate reflection probes if diffuse probes are being built.
    static float a = 0.0f;
    a += state.time.delta_seconds;
    if (a < 30.0f)
    {
        return;
    }


    if (light_probe_system->is_regenerating())
    {
        return;
    }

    // Look for reflection probes that need to be updated.
    {
        profile_marker(profile_colors::render, "Reflection Probes");

        for (render_reflection_probe* probe : reflection_probes)
        {
            debug_system->add_obb(probe->get_bounds(), color::blue);

            // If dirty, regenerate this probe.
            if (probe->is_dirty() && m_probes_regenerating < k_probe_regenerations_per_frame)
            {
                regenerate_probe(probe, m_probes_regenerating);
                m_probes_regenerating++;
            }
        }
    }
}

}; // namespace ws
