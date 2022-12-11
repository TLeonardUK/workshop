// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/renderer.h"
#include "workshop.render_interface/ri_interface.h"

#include "workshop.renderer/systems/render_system_clear.h"
#include "workshop.renderer/systems/render_system_resolve_backbuffer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/assets/shader/shader.h"
#include "workshop.renderer/assets/shader/shader_loader.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_swapchain.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_sampler.h"

#include "workshop.assets/asset_manager.h"

#include "workshop.windowing/window.h"

#include "workshop.core/filesystem/virtual_file_system.h"

#include "workshop.core/async/async.h"
#include "workshop.core/perf/profile.h"
#include "workshop.core/hashing/hash.h"

namespace ws {

renderer::renderer(ri_interface& rhi, window& main_window, asset_manager& asset_manager)
    : m_render_interface(rhi)
    , m_window(main_window)
    , m_asset_manager(asset_manager)
{
    // Note: Order is important here, this is the order the 
    //       stages will be added to the render graph in.
    m_systems.push_back(std::make_unique<render_system_clear>(*this));
    m_systems.push_back(std::make_unique<render_system_resolve_backbuffer>(*this));
}

void renderer::register_init(init_list& list)
{
    list.add_step(
        "Load Shaders",
        [this, &list]() -> result<void> { return create_shaders(); },
        [this, &list]() -> result<void> { return destroy_shaders(); }
    );
    list.add_step(
        "Renderer Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );

    for (auto& system : m_systems)
    {
        system->register_init(list);
    }

    list.add_step(
        "Build Render Graph",
        [this, &list]() -> result<void> { return create_render_graph(); },
        [this, &list]() -> result<void> { return destroy_render_graph(); }
    );
}

result<void> renderer::create_shaders()
{
    // Register the shader loader with the asset manager.
    m_asset_manager.register_loader(std::make_unique<shader_loader>(m_render_interface, *this));

    // Queue all shader assets for load.
    std::vector<std::string> potential_files = virtual_file_system::get().list("data:shaders", virtual_file_system_path_type::file);
    for (std::string& file : potential_files)
    {
        std::string extension = virtual_file_system::get_extension(file.c_str());
        if (extension == asset_manager::k_asset_extension)
        {
            m_shader_assets.push_back(m_asset_manager.request_asset<shader>(file.c_str(), 0));
        }
    }    

    // Wait for all loads to complete.
    for (auto& shader_ptr : m_shader_assets)
    {
        shader_ptr.wait_for_load();
        if (shader_ptr.get_state() == asset_loading_state::failed)
        {
            db_fatal(renderer, "Failed to load required shader: %s", shader_ptr.get_path().c_str());
        }
    }

    return true;
}

result<void> renderer::destroy_shaders()
{
    db_assert_message(m_effects.empty(), "Resource leak, destroying register but all effects have not been unregistered.");
    db_assert_message(m_param_block_archetypes.empty(), "Resource leak, destroying register but all param block archetypes have not been unregistered.");

    // Ensure all render jobs have completed.
    m_render_job_task.wait();
    m_swapchain = nullptr;

    return true;
}

result<void> renderer::create_resources()
{ 
    m_swapchain = m_render_interface.create_swapchain(m_window, "Renderer Swapchain");
    if (!m_swapchain)
    {
        return false;
    }

    // Recreates any targets that change based on swapchain size.
    if (result<void> ret = recreate_resizable_targets(); !ret)
    {
        return ret;
    }

    // TODO: create our bindless arrays.

    return true;
}

result<void> renderer::destroy_resources()
{
    // Ensure all render jobs have completed.
    m_render_job_task.wait();

    // Nuke all resizable targets.
    m_gbuffer_sampler = nullptr;
    m_depth_texture = nullptr;
    for (size_t i = 0; i < k_gbuffer_count; i++)
    {
        m_gbuffer_textures[i] = nullptr;
    }

    // Nuke swapchain.
    m_swapchain = nullptr;

    return true;
}

result<void> renderer::recreate_resizable_targets()
{
    // Create depth buffer.
    ri_texture::create_params params;
    params.width = m_window.get_width();
    params.height = m_window.get_height();
    params.dimensions = ri_texture_dimension::texture_2d;
    params.format = ri_texture_format::D24_UNORM_S8_UINT;
    params.is_render_target = true;
    m_depth_texture = m_render_interface.create_texture(params, "depth buffer");

    // Create each gbuffer.
    params.format = ri_texture_format::R16G16B16A16_UNORM;
    m_gbuffer_textures[0] = m_render_interface.create_texture(params, "gbuffer[0] rgb:diffuse a:flags");

    params.format = ri_texture_format::R16G16B16A16_FLOAT;
    m_gbuffer_textures[1] = m_render_interface.create_texture(params, "gbuffer[1] rgb:world normal a:roughness");
    m_gbuffer_textures[2] = m_render_interface.create_texture(params, "gbuffer[2] rgb:world position a:metallic");

    // Create sampler used for reading the gbuffer.
    ri_sampler::create_params sampler_params;
    m_gbuffer_sampler = m_render_interface.create_sampler(sampler_params, "gbuffer sampler");

    // TODO: create our bindless arrays.
    //m_render_interface.create_bindless_array(ri_bindless_array_type::texture_1d, "bindless table: texture1d");

    return true;
}

ri_interface& renderer::get_render_interface()
{
    return m_render_interface;
}

ri_texture& renderer::get_next_backbuffer()
{
    return m_swapchain->next_backbuffer();
}

render_output renderer::get_gbuffer_output()
{
    render_output output;
    output.depth_target = m_depth_texture.get();
    output.color_targets.push_back(m_gbuffer_textures[0].get());
    output.color_targets.push_back(m_gbuffer_textures[1].get());
    output.color_targets.push_back(m_gbuffer_textures[2].get());
    return output;
}

renderer::effect_id renderer::register_effect(std::unique_ptr<render_effect>&& effect)
{
    effect_id id = m_id_counter++;
    m_effects[id] = std::move(effect);
    return id;
}

void renderer::unregister_effect(effect_id id)
{
    // TODO: Handle if the effect is in use.

    if (auto iter = m_effects.find(id); iter != m_effects.end())
    {
        m_effects.erase(iter);
    }
}

render_effect* renderer::get_effect(effect_id id)
{
    if (auto iter = m_effects.find(id); iter != m_effects.end())
    {
        return iter->second.get();
    }
    return nullptr;
}

render_effect::technique* renderer::get_technique(const char* name, const std::unordered_map<std::string, std::string>& parameters)
{
    // Start with list of all techniques.
    std::vector<render_effect::technique*> techniques;

    // Get a list of all potential techniques for matching effect names. We can have
    // multiple effects with the same name, but different techniques (eg. the game extending
    // an effect defined in the engine).
    for (auto& pair : m_effects)
    {
        if (pair.second->name == name)
        {
            for (render_effect::technique& technique : pair.second->techniques)
            {
                techniques.push_back(&technique);
            }
        }
    }

    // Remove all techniques that do not accept the parameters we are passing.
    auto iter = std::remove_if(techniques.begin(), techniques.end(), [&parameters](const render_effect::technique* technique) -> bool {

        bool valid = true;

        // Go through all expected parameters.
        for (auto& expected_value : parameters)
        {
            // Find parameter in the techniques variation list.
            for (const render_effect::variation_parameter& param : technique->variation_parameters)
            {
                if (param.name == expected_value.first)
                {
                    // Check the value provided is in the list of accepted values.
                    valid = (std::find(param.values.begin(), param.values.end(), expected_value.second) != param.values.end());
                    break;
                }
            }

            if (!valid)
            {
                valid = false;
                break;
            }
        }

        return !valid;

    });

    techniques.erase(iter, techniques.end());

    // If we have none left we have failed.
    if (techniques.size() == 0)
    {
        return nullptr;
    }
    // If we have one left, we are done.
    else if (techniques.size() == 1)
    {
        return techniques[0];
    }
    // Otherwise we don't have enough information to disambiguate the correct
    // effect so provide an error log to track things down.
    else
    {
        db_warning(renderer, "Attempt to find technique for effect '%s' with the following parameters:", name);
        for (auto& pair : parameters)
        {
            db_warning(renderer, "\t%s = %s", pair.first.c_str(), pair.second.c_str());
        }

        db_warning(renderer, "Provided ambiguous results. Could select any of the following:");
        for (render_effect::technique* instance : techniques)
        {
            db_warning(renderer, "\t%s", instance->name.c_str());
            for (auto& pair : instance->variation_parameters)
            {
                db_warning(renderer, "\t\t%s = [%s]", pair.name.c_str(), string_join(pair.values, ", ").c_str());
            }
        }
        
        return nullptr;
    }
}

renderer::param_block_archetype_id renderer::register_param_block_archetype(const char* name, ri_data_scope scope, const ri_data_layout& layout)
{
    // Generate a hash for the archetype so we can 
    // do a quick look up to determine if it already exists.
    size_t hash = 0;
    hash_combine(hash, scope);
    for (const ri_data_layout::field& field : layout.fields)
    {
        hash_combine(hash, field.data_type);
        hash_combine(hash, field.name);
    }

    for (auto& pair : m_param_block_archetypes)
    {
        if (pair.second.hash == hash)
        {
            pair.second.register_count++;
            return pair.first;
        }
    }

    param_block_state state;
    state.name = name;
    state.hash = hash;
    state.register_count = 1;

    ri_param_block_archetype::create_params params;
    params.layout = layout;
    params.scope = scope;

    state.instance = m_render_interface.create_param_block_archetype(params, name);
    if (state.instance == nullptr)
    {
        db_error(asset, "Failed to create param block archetype '%s'.", name);
        return invalid_id;
    }

    param_block_archetype_id id = m_id_counter++;
    m_param_block_archetypes[id] = std::move(state);
    return id;
}

void renderer::unregister_param_block_archetype(param_block_archetype_id id)
{
    // TODO: Handle if the param block archetype is in use.

    if (auto iter = m_param_block_archetypes.find(id); iter != m_param_block_archetypes.end())
    {
        if (--iter->second.register_count == 0)
        {
            m_param_block_archetypes.erase(iter);
        }
    }
}

ri_param_block_archetype* renderer::get_param_block_archetype(param_block_archetype_id id)
{
    if (auto iter = m_param_block_archetypes.find(id); iter != m_param_block_archetypes.end())
    {
        return iter->second.instance.get();
    }
    return nullptr;    
}

void renderer::render_state(render_world_state& state)
{
    profile_marker(profile_colors::render, "render frame %zi", (size_t)state.time.frame_count);

    // Update all systems in parallel.
    parallel_for("step render systems", task_queue::standard, m_systems.size(), [this, &state](size_t index) {
        profile_marker(profile_colors::render, "step render system: %s", m_systems[index]->name.c_str());
        m_systems[index]->step(state);
    });

    // Begin the new frame.
    m_render_interface.new_frame();

    // Render each view.
    std::vector<std::vector<render_pass::generated_state>> view_generated_states;
    view_generated_states.resize(state.views.size());

    parallel_for("render views", task_queue::standard, state.views.size(), [this, &state, &view_generated_states](size_t index) {
        profile_marker(profile_colors::render, "render view: %s", state.views[index].name.c_str());
        render_single_view(state, state.views[index], view_generated_states[index]);
    });

    // Dispatch all generated command lists.
    {
        profile_marker(profile_colors::render, "dispatch command lists");

        ri_command_queue& graphics_command_queue = m_render_interface.get_graphics_queue();

        for (std::vector<render_pass::generated_state>& generated_state_list : view_generated_states)
        {
            for (render_pass::generated_state& state : generated_state_list)
            {
                for (auto& graphics_list : state.graphics_command_lists)
                {
                    graphics_command_queue.execute(*graphics_list);
                }
            }
        }
    }

    // Present, we're done with this frame!
    {
        profile_marker(profile_colors::render, "present");
        m_swapchain->present();
    }
}

void renderer::render_single_view(render_world_state& state, render_view& view, std::vector<render_pass::generated_state>& output)
{
    profile_marker(profile_colors::render, "render view");

    // Generate command lists for all nodes in parallel.
    std::vector<render_graph::node*> nodes;
    m_render_graph->get_active(nodes);
    
    output.resize(nodes.size());
    
    parallel_for("generate render passs", task_queue::standard, nodes.size(), [this, &state, &output, &nodes, &view](size_t index) {
        render_graph::node* node = nodes[index];
        
        profile_marker(profile_colors::render, "generate render pass: %s", node->pass->name.c_str());        
        //get_technique(node->pass->effect_name.c_str(), node->pass->effect_variations);

        node->pass->generate(*this, output[index], view);
    });
}

void renderer::render_job()
{
    std::unique_ptr<render_world_state> state;
    while (true)
    {
        // Try and pop the next pending frame.
        {
            std::unique_lock lock(m_pending_frames_mutex);
            if (m_pending_frames.size() > 0)
            {
                state = std::move(m_pending_frames.front());
                m_pending_frames.pop();
            }
            else
            {
                m_render_job_active = false;
                break;
            }
        }

        // Render it.
        render_state(*state);

        // Update bookkeeping to mark this frame as completed.
        {
            std::unique_lock lock(m_pending_frames_mutex);
            m_frames_in_flight.fetch_sub(1);
            m_pending_frame_convar.notify_all();
        }
    }
}

void renderer::step(std::unique_ptr<render_world_state>&& state)
{
    // Push this frame into the queue, and optionally wait if 
    // queue depth has gotten too great.
    bool start_new_render_job = false;
    {
        std::unique_lock lock(m_pending_frames_mutex);
        m_pending_frames.push(std::move(state));
        m_frames_in_flight.fetch_add(1);

        // Wait for previous frames to complete if depth is high enough.
        while (m_frames_in_flight.load() >= k_frame_depth)
        {
            profile_marker(profile_colors::render, "wait for render");
            m_pending_frame_convar.wait(lock);
        }

        // If previous render job has completed we need to start another to
        // process the queued frame.
        if (!m_render_job_active)
        {
            m_render_job_active = true;
            start_new_render_job = true;
        }
    }

    // If previous render job has finished, start a new one.
    if (start_new_render_job)
    {
        m_render_job_task = async("Render Job", task_queue::standard, [this]() {
            render_job();    
        });
    }
}

result<void> renderer::create_render_graph()
{
    m_render_graph = std::make_unique<render_graph>();

    for (auto& system : m_systems)
    {
        system->create_graph(*m_render_graph);
    }

    std::vector<render_graph::node*> nodes;
    m_render_graph->get_nodes(nodes);

    for (auto& node : nodes)
    {
        if (result<void> ret = node->pass->create_resources(*this); !ret)
        {
            return ret;
        }
    }

    return true;
}

result<void> renderer::destroy_render_graph()
{
    std::vector<render_graph::node*> nodes;
    m_render_graph->get_nodes(nodes);

    for (auto& node : nodes)
    {
        if (result<void> ret = node->pass->destroy_resources(*this); !ret)
        {
            return ret;
        }
    }

    m_render_graph = nullptr;

    return true;
}

}; // namespace ws
