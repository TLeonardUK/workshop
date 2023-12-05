// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_raytrace_scene.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/passes/render_pass_raytracing.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"

#include <random>

namespace ws {

render_system_raytrace_scene::render_system_raytrace_scene(renderer& render)
    : render_system(render, "raytrace scene")
{
}

void render_system_raytrace_scene::register_init(init_list& list)
{
    list.add_step(
        "Raytrace Scene Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> render_system_raytrace_scene::create_resources()
{
    ri_interface& render_interface = m_renderer.get_render_interface();

    const render_options& options = m_renderer.get_options();

    ri_texture::create_params texture_params;
    texture_params.width = m_renderer.get_display_width();
    texture_params.height = m_renderer.get_display_height();
    texture_params.dimensions = ri_texture_dimension::texture_2d;
    texture_params.format = ri_texture_format::R16G16B16A16_FLOAT; 
    texture_params.is_render_target = false;
    texture_params.allow_unordered_access = true;
    texture_params.optimal_clear_color = color(1.0f, 0.0f, 0.0f, 0.0f);
    m_scene_texture = render_interface.create_texture(texture_params, "raytrace scene buffer");

    return true;
}

void render_system_raytrace_scene::swapchain_resized()
{
    bool result = create_resources();
    db_assert(result);
}

result<void> render_system_raytrace_scene::destroy_resources()
{
    return true;
}

ri_texture& render_system_raytrace_scene::get_output_buffer()
{
    return *m_scene_texture;
}

void render_system_raytrace_scene::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal))
    {
        return;
    }

    if (m_renderer.get_visualization_mode() != visualization_mode::raytraced_scene)
    {
        return;
    }

    render_system_lighting* lighting_system = m_renderer.get_system<render_system_lighting>();

    const render_options& options = m_renderer.get_options();

    if (!options.raytracing_enabled || 
        !m_renderer.get_render_interface().check_feature(ri_feature::raytracing))
    {
        return;
    }

    ri_param_block* raytrace_scene_parameters = view.get_resource_cache().find_or_create_param_block(this, "raytrace_scene_parameters");

    raytrace_scene_parameters->set("scene_tlas", m_renderer.get_scene_tlas());
    raytrace_scene_parameters->set("scene_tlas_metadata", *m_renderer.get_scene_tlas().get_metadata_buffer());
    raytrace_scene_parameters->set("output_texture", ri_texture_view(m_scene_texture.get(), 0, 0), true);

#if 0
    // Fake view
    vector3 fake_position = vector3(1028.73f, 222.09f, -798.79f);
    quat fake_rotation = quat(-0.01f, 0.69f, -0.01f, 0.72f);

    matrix4 view_matrix = matrix4::look_at(
        fake_position,
        fake_position + (vector3::forward * fake_rotation),
        vector3::up);
    matrix4 projection_matrix = matrix4::perspective(
        math::radians(45.0f),
        1920.0f / 1080.0f,
        0.1f,
        100000.0f);

    static std::unique_ptr<ri_param_block> fake_view_info = m_renderer.get_param_block_manager().create_param_block("view_info");
    fake_view_info->set("view_z_near", 0.1f);
    fake_view_info->set("view_z_far", 100000.0f);
    fake_view_info->set("view_world_position", fake_position);
    fake_view_info->set("view_dimensions", vector2((float)m_scene_texture->get_width(), (float)m_scene_texture->get_height()));
    fake_view_info->set("view_matrix", view_matrix);
    fake_view_info->set("projection_matrix", projection_matrix);
    fake_view_info->set("inverse_view_matrix", view_matrix.inverse());
    fake_view_info->set("inverse_projection_matrix", projection_matrix.inverse());
#endif

    // Composite the transparent geometry onto the light buffer.
    std::unique_ptr<render_pass_raytracing> resolve_pass = std::make_unique<render_pass_raytracing>();
    resolve_pass->name = "raytrace scene";
    resolve_pass->system = this;
    resolve_pass->unordered_access_textures.push_back(m_scene_texture.get());
    resolve_pass->dispatch_size = vector3i((int)m_scene_texture->get_width(), (int)m_scene_texture->get_height(), 1);
    resolve_pass->technique = m_renderer.get_effect_manager().get_technique("raytrace_scene", {});
#if 0
    resolve_pass->param_blocks.push_back(fake_view_info.get());
#else
    resolve_pass->param_blocks.push_back(view.get_view_info_param_block());
#endif
    resolve_pass->param_blocks.push_back(raytrace_scene_parameters);
    resolve_pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    resolve_pass->param_blocks.push_back(&lighting_system->get_resolve_param_block(view));
    graph.add_node(std::move(resolve_pass));
}

void render_system_raytrace_scene::step(const render_world_state& state)
{
}

}; // namespace ws
