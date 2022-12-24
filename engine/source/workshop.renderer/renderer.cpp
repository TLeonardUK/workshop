// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/renderer.h"
#include "workshop.render_interface/ri_interface.h"

#include "workshop.renderer/systems/render_system_clear.h"
#include "workshop.renderer/systems/render_system_resolve_backbuffer.h"
#include "workshop.renderer/systems/render_system_imgui.h"
#include "workshop.renderer/systems/render_system_geometry.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_scene_manager.h"
#include "workshop.renderer/assets/shader/shader.h"
#include "workshop.renderer/assets/shader/shader_loader.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_swapchain.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_sampler.h"
#include "workshop.render_interface/ri_param_block.h"

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
    m_systems.push_back(std::make_unique<render_system_geometry>(*this));
    m_systems.push_back(std::make_unique<render_system_imgui>(*this));
    m_systems.push_back(std::make_unique<render_system_resolve_backbuffer>(*this));

    m_effect_manager = std::make_unique<render_effect_manager>(*this, m_asset_manager);
    m_param_block_manager = std::make_unique<render_param_block_manager>(*this);
    m_scene_manager = std::make_unique<render_scene_manager>(*this);
}

void renderer::register_init(init_list& list)
{
    m_param_block_manager->register_init(list);
    m_effect_manager->register_init(list);
    m_scene_manager->register_init(list);

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

    return true;
}

result<void> renderer::destroy_resources()
{
    // Ensure all render jobs have completed.
    m_render_job_task.wait();

    // Nuke all resizable targets.
    m_gbuffer_param_block = nullptr;
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

    // Create param block describing the above.
    m_gbuffer_param_block = m_param_block_manager->create_param_block("gbuffer");
    m_gbuffer_param_block->set("gbuffer0_texture", *m_gbuffer_textures[0]);
    m_gbuffer_param_block->set("gbuffer1_texture", *m_gbuffer_textures[1]);
    m_gbuffer_param_block->set("gbuffer2_texture", *m_gbuffer_textures[2]);
    m_gbuffer_param_block->set("gbuffer_sampler", *m_gbuffer_sampler);

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

render_output renderer::get_swapchain_output()
{
    if (m_current_backbuffer == nullptr)
    {
        m_current_backbuffer = &get_next_backbuffer();
    }

    render_output output;
    output.depth_target = nullptr;
    output.color_targets.push_back(m_current_backbuffer);
    return output;
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

ri_param_block* renderer::get_gbuffer_param_block()
{
    return m_gbuffer_param_block.get();
}

render_param_block_manager& renderer::get_param_block_manager()
{
    return *m_param_block_manager;
}

render_effect_manager& renderer::get_effect_manager()
{
    return *m_effect_manager;
}

render_scene_manager& renderer::get_scene_manager()
{
    return *m_scene_manager;
}

void renderer::render_state(render_world_state& state)
{
    ri_command_queue& graphics_command_queue = m_render_interface.get_graphics_queue();
    ri_command_queue& copy_command_queue = m_render_interface.get_copy_queue();

    profile_marker(profile_colors::render, "frame %zi", (size_t)state.time.frame_count);

    // Grab the next backbuffer, and wait for gpu if pipeline is full.
    m_current_backbuffer = &get_next_backbuffer();

    // Update all systems in parallel.
    parallel_for("step render systems", task_queue::standard, m_systems.size(), [this, &state](size_t index) {
        profile_marker(profile_colors::render, "step render system: %s", m_systems[index]->name.c_str());
        m_systems[index]->step(state);
    });

    // Begin the new frame.
    m_render_interface.begin_frame();

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

        for (size_t i = 0; i < state.views.size(); i++)
        {            
            std::vector<render_pass::generated_state>& generated_state_list = view_generated_states[i];
            render_view& view = state.views[i];

            profile_gpu_marker(graphics_command_queue, profile_colors::gpu_view, "render view: %s", view.name.c_str());

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

    // End the frame.
    m_render_interface.end_frame();
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
