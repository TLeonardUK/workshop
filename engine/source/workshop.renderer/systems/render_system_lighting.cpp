// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/passes/render_pass_fullscreen.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/objects/render_directional_light.h"
#include "workshop.renderer/objects/render_point_light.h"
#include "workshop.renderer/objects/render_spot_light.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/render_resource_cache.h"

namespace ws {

render_system_lighting::render_system_lighting(renderer& render)
    : render_system(render, "lighting")
{
}

void render_system_lighting::register_init(init_list& list)
{
    list.add_step(
        "Renderer Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> render_system_lighting::create_resources()
{
    ri_interface& render_interface = m_renderer.get_render_interface();

    ri_texture::create_params texture_params;
    texture_params.width = m_renderer.get_display_width();
    texture_params.height = m_renderer.get_display_height();
    texture_params.dimensions = ri_texture_dimension::texture_2d;
    texture_params.format = ri_texture_format::R16G16B16A16_FLOAT;
    texture_params.is_render_target = true;
    m_lighting_buffer = render_interface.create_texture(texture_params, "lighting buffer");
    
    m_lighting_output.depth_target = nullptr;
    m_lighting_output.color_targets.push_back(m_lighting_buffer.get());

    return true;
}

result<void> render_system_lighting::destroy_resources()
{
    return true;
}

ri_texture& render_system_lighting::get_lighting_buffer()
{
    return *m_lighting_buffer;
}

void render_system_lighting::create_graph(render_graph& graph)
{
    // Run compute shader to cluster our lights.
    // TODO

    // Generate the light accumulation buffer.
    std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
    pass->name = "resolve lighting";
    pass->technique = m_renderer.get_effect_manager().get_technique("resolve_lighting", {});
    pass->output = m_lighting_output;
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());

    m_render_pass = pass.get();

    graph.add_node(std::move(pass));
}

void render_system_lighting::step(const render_world_state& state)
{
}

void render_system_lighting::step_view(const render_world_state& state, render_view& view)
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    
    render_batch_instance_buffer* instance_buffer = view.get_resource_cache().find_or_create_instance_buffer(this);
    ri_param_block* resolve_param_block = view.get_resource_cache().find_or_create_param_block(this, "resolve_lighting_parameters");

    // Grab all lights that can directly effect the frustum.
    // TODO: Doing an octtree query should be faster than this, reconsider.
    std::vector<render_light*> visible_lights;
    for (auto& light : scene_manager.get_directional_lights())
    {
        if (view.is_object_visible(light))
        {
            visible_lights.push_back(light);
        }
    }
    for (auto& light : scene_manager.get_point_lights())
    {
        if (view.is_object_visible(light))
        {
            visible_lights.push_back(light);
        }
    }
    for (auto& light : scene_manager.get_spot_lights())
    {
        if (view.is_object_visible(light))
        {
            visible_lights.push_back(light);
        }
    }

    // Fill in the buffer.
    int total_lights = 0;

    for (render_light* light : visible_lights)
    {
        ri_param_block* block = light->get_light_state_param_block();

        size_t index, offset;
        block->get_table(index, offset);
        instance_buffer->add(index, offset);

        total_lights++;
        if (total_lights >= k_max_lights)
        {
            break;
        }
    }

    instance_buffer->commit();

    // Update the number of lights we have in the buffer.
    resolve_param_block->set("light_count", total_lights);
    resolve_param_block->set("light_buffer", instance_buffer->get_buffer());
    resolve_param_block->set("view_world_position", view.get_local_location());
}

}; // namespace ws
