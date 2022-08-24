// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

#include "workshop.core/async/task_scheduler.h"

#include "workshop.renderer/render_world_state.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/render_system.h"

#include <queue>

namespace ws {

class render_swapchain;
class render_interface;
class window;
class render_target;
class render_effect;

// ================================================================================================
//  Provides an abstracted interface for all rendering behaviour in the engine 
// ================================================================================================
class renderer
{
public:

    renderer() = delete;
    renderer(render_interface& rhi, window& main_window);

    // Registers all the steps required to initialize the renderer.
    void register_init(init_list& list);

    // Takes the world state and dispatches a new render task which
    // will draw the state to screen.
    void step(std::unique_ptr<render_world_state>&& state);

    // Gets a specific render system by type.
    template <typename T>
    T* get_system()
    {
        for (auto& system : m_systems)
        {
            if (typeid(T) == typeid(*system))
            {
                return system.get();
            }
        }
        return nullptr;
    }

    // Gets the next backbuffer to render to. May block if all swapchain
    // buffers are in use.
    render_target& get_next_backbuffer();

    // Gets an effect by name.
    render_effect* get_effect(const char* name);

private:

    result<void> create_resources();
    result<void> destroy_resources();

    // Rebuilds the render graph. This should only occur on start or
    // when settings have changed that require the graph to be rebuilt
    // (eg. quality settings have changed).
    // Its not expected that this should be called regularly.
    void rebuild_render_graph();

    // Called to run one or more render jobs currently in the queue.
    void render_job();

    // Renders the given world state.
    void render_state(render_world_state& state);

    // Converts a render node into command lists and other state that can be
    // used to execute them.
    void generate_node(render_graph::node& node, render_graph_node_generated_state& state);

private:

    constexpr static inline size_t k_frame_depth = 3;

    render_interface& m_render_interface;
    window& m_window;

    std::unordered_map<std::string, std::unique_ptr<render_effect>> m_effects;

    std::unique_ptr<render_swapchain> m_swapchain;

    std::vector<std::unique_ptr<render_system>> m_systems;

    std::unique_ptr<render_graph> m_render_graph;

    std::queue<std::unique_ptr<render_world_state>> m_pending_frames;
    std::mutex m_pending_frames_mutex;
    std::condition_variable m_pending_frame_convar;

    std::atomic_size_t m_frames_in_flight { 0 };
    std::atomic_bool m_render_job_active { false };

    task_handle m_render_job_task;

};

}; // namespace ws
