// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/renderer.h"
#include "workshop.render_interface/render_interface.h"

#include "workshop.renderer/systems/render_system_geometry.h"
#include "workshop.renderer/systems/render_system_copy_to_backbuffer.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/render_effect.h"

#include "workshop.render_interface/render_interface.h"
#include "workshop.render_interface/render_swapchain.h"
#include "workshop.render_interface/render_command_queue.h"
#include "workshop.render_interface/render_command_list.h"

#include "workshop.windowing/window.h"

#include "workshop.core/async/async.h"
#include "workshop.core/perf/profile.h"

namespace ws {

renderer::renderer(render_interface& rhi, window& main_window)
    : m_render_interface(rhi)
    , m_window(main_window)
{
    // Note: Order is important here, this is the order the 
    //       stages will be added to the render graph in.
    m_systems.push_back(std::make_unique<render_system_geometry>(*this));
    m_systems.push_back(std::make_unique<render_system_copy_to_backbuffer>(*this));
}

void renderer::register_init(init_list& list)
{
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
        [this, &list]() -> result<void> { rebuild_render_graph(); return true; },
        [this, &list]() -> result<void> { m_render_graph = nullptr; return true; }
    );
}

result<void> renderer::create_resources()
{
    m_swapchain = m_render_interface.create_swapchain(m_window, "Renderer Swapchain");
    if (!m_swapchain)
    {
        return false;
    }

    // Load all effects.

    return true;
}

result<void> renderer::destroy_resources()
{
    // Ensure all render jobs have completed.
    m_effects.clear();
    m_render_job_task.wait();
    m_swapchain = nullptr;

    return true;
}

render_target& renderer::get_next_backbuffer()
{
    return m_swapchain->next_backbuffer();
}

render_effect* renderer::get_effect(const char* name)
{
    if (auto iter = m_effects.find(name); iter != m_effects.end())
    {
        return iter->second.get();
    }
    return nullptr;
}

void renderer::generate_node(render_graph::node& node, render_graph_node_generated_state& state)
{
}

void renderer::render_state(render_world_state& state)
{
    profile_marker(profile_colors::render, "render frame %zi", (size_t)state.time.frame_count);

    // Update each systme.
    {
        profile_marker(profile_colors::render, "step systems");

        for (auto& system : m_systems)
        {
            system->step(state);
        }
    }

    // Begin the new frame.
    render_command_queue& graphics_command_queue = m_render_interface.get_graphics_queue();
    m_render_interface.new_frame();

    // Generate command lists for all nodes in parallel.
    std::vector<render_graph::node*>& nodes = m_render_graph->get_flattened_nodes();
    std::vector<task_handle> tasks;
    tasks.resize(nodes.size());

    std::vector<render_graph_node_generated_state> generated_state;
    generated_state.resize(nodes.size());

    for (size_t i = 0; i < nodes.size(); i++)
    {
        tasks[i] = task_scheduler::get().create_task("render node generation", task_queue::standard, [this, &nodes, &generated_state, i]() {
            generate_node(*nodes[i], generated_state[i]);
        });
    }

    for (size_t i = 0; i < nodes.size(); i++)
    {
        for (render_graph::node_id dependency : nodes[i]->dependencies)
        {
            tasks[i].add_dependency(tasks[static_cast<int>(dependency)]);
        }
    }

    task_scheduler::get().dispatch_tasks(tasks);
    task_scheduler::get().wait_for_tasks(tasks, true);

    // Execute all command lists in node order.
    for (render_graph_node_generated_state& state : generated_state)
    {
        for (auto& graphics_list : state.graphics_command_lists)
        {
            graphics_command_queue.execute(*graphics_list);
        }
    }

    // Present, we're done with this frame!
    m_swapchain->present();

    // TODO: General render queues/etc from the world state.
    // This is debug for the moment.
    /*
    render_target& buffer_rt = m_swapchain->next_backbuffer();

    render_command_list& list = graphics_command_queue.alloc_command_list();
    list.open();
    list.barrier(buffer_rt, render_resource_state::present, render_resource_state::render_target);
    list.clear(buffer_rt, color::light_blue);
    list.barrier(buffer_rt, render_resource_state::render_target, render_resource_state::present);
    list.close();
    */
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

void renderer::rebuild_render_graph()
{
    m_render_graph = std::make_unique<render_graph>();

    for (auto& system : m_systems)
    {
        system->create_graph(*m_render_graph);
    }

    m_render_graph->flatten();
}

}; // namespace ws
