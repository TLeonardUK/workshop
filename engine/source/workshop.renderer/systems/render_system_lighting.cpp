// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/systems/render_system_shadows.h"
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

void render_system_lighting::build_graph(render_graph& graph, const render_world_state& state, render_view& view)
{
    if (!view.has_flag(render_view_flags::normal))
    {
        return;
    }

    render_system_shadows* shadow_system = m_renderer.get_system<render_system_shadows>();

    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    render_batch_instance_buffer* light_instance_buffer = view.get_resource_cache().find_or_create_instance_buffer(this);
    render_batch_instance_buffer* shadow_map_instance_buffer = view.get_resource_cache().find_or_create_instance_buffer(this + 1);
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

    // Fill in the indirection buffers.
    int total_lights = 0;
    int total_shadow_maps = 0;

    for (render_light* light : visible_lights)
    {
        ri_param_block* light_state_block = light->get_light_state_param_block();
        render_system_shadows::shadow_info& shadows = shadow_system->find_or_create_shadow_info(light->get_id(), view.get_id());

        // Make sure we have space left in the lists.
        if (total_lights + 1 >= k_max_lights ||
            total_shadow_maps + shadows.cascades.size() >= k_max_shadow_maps)
        {
            break;
        }

        // Add light instance to buffer.
        size_t index, offset;
        light_state_block->set("shadow_map_start_index", total_shadow_maps);
        light_state_block->set("shadow_map_count", (int)shadows.cascades.size());
        light_state_block->get_table(index, offset);
        light_instance_buffer->add(index, offset);

        // Add each shadow info to buffer.
        for (render_system_shadows::cascade_info& cascade : shadows.cascades)
        {
            cascade.shadow_map_state_param_block->get_table(index, offset);
            shadow_map_instance_buffer->add(index, offset);
        }

        total_lights++;
        total_shadow_maps += shadows.cascades.size();
    }

    light_instance_buffer->commit();
    shadow_map_instance_buffer->commit();

    // Update the number of lights we have in the buffer.
    resolve_param_block->set("light_count", total_lights);
    resolve_param_block->set("light_buffer", light_instance_buffer->get_buffer());
    resolve_param_block->set("shadow_map_count", total_shadow_maps);
    resolve_param_block->set("shadow_map_buffer", shadow_map_instance_buffer->get_buffer());
    resolve_param_block->set("shadow_map_sampler", *m_renderer.get_default_sampler(default_sampler_type::shadow_map));
    resolve_param_block->set("view_world_position", view.get_local_location());

    // Add pass to run compute shader to cluster our lights.
    // TODO

    // Add pass to generate the light accumulation buffer.
    std::unique_ptr<render_pass_fullscreen> pass = std::make_unique<render_pass_fullscreen>();
    pass->name = "resolve lighting";
    pass->system = this;
    pass->technique = m_renderer.get_effect_manager().get_technique("resolve_lighting", {});
    pass->output = m_lighting_output;
    pass->param_blocks.push_back(m_renderer.get_gbuffer_param_block());
    pass->param_blocks.push_back(resolve_param_block);

    graph.add_node(std::move(pass));
}

void render_system_lighting::step(const render_world_state& state)
{
}

}; // namespace ws
