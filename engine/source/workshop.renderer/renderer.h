// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/containers/command_queue.h"

#include "workshop.renderer/render_world_state.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/render_system.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_output.h"
#include "workshop.renderer/render_scene_manager.h"
#include "workshop.renderer/render_command_queue.h"
#include "workshop.render_interface/ri_types.h"
#include "workshop.render_interface/ri_param_block.h"

#include "workshop.assets/asset_manager.h"

#include <queue>

namespace ws {

class ri_swapchain;
class ri_interface;
class ri_texture;
class ri_sampler;
class ri_param_block_archetype; 
class render_view;
class render_param_block_manager;
class render_effect_manager;
class render_scene_manager;
class asset_manager;
class window;
class shader;

// ================================================================================================
//  Provides an abstracted interface for all rendering behaviour in the engine 
// ================================================================================================
class renderer
{
public:

    renderer() = delete;
    renderer(ri_interface& rhi, window& main_window, asset_manager& asset_manager);

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

    // Gets the render interface this renderer is using.
    ri_interface& get_render_interface();

    // Gets the next backbuffer to render to. May block if all swapchain
    // buffers are in use.
    ri_texture& get_next_backbuffer();

    // Gets render output for swapchain for the current frame. This will change each frame.
    render_output get_swapchain_output();

    // Gets render output for drawing to the gbuffer.
    render_output get_gbuffer_output();

    // Gets the param block that contains all the information needed for reading
    // and writing to the gbuffer.
    ri_param_block* get_gbuffer_param_block();

    // Gets the param block archetype repository.
    render_param_block_manager& get_param_block_manager();

    // Gets the effect repository.
    render_effect_manager& get_effect_manager();

    // Gets the manager that handles the objects contained in the render scene.
    render_scene_manager& get_scene_manager();

    // Gets the current command queue that should be written to.
    // This is only valid for the current frame, do not try to cache this.
    render_command_queue& get_command_queue();

    // Gets the next opaque id of a render object. This is thread safe.
    render_object_id next_render_object_id();

private:

    result<void> create_resources();
    result<void> destroy_resources();

    result<void> register_asset_loaders();
    result<void> unregister_asset_loaders();

    result<void> recreate_resizable_targets();

    // Rebuilds the render graph. This should only occur on start or
    // when settings have changed that require the graph to be rebuilt
    // (eg. quality settings have changed).
    // Its not expected that this should be called regularly.
    result<void> create_render_graph();

    // Tears down the render grpah.
    result<void> destroy_render_graph();

    // Called to run one or more render jobs currently in the queue.
    void render_job();

    // Renders the given world state.
    void render_state(render_world_state& state);

    // Executes all the commands stored in the given world state.
    void process_render_commands(render_world_state& state);

    // Renders the give view.
    void render_single_view(render_world_state& state, render_view& view, std::vector<render_pass::generated_state>& output);

private:

    // How many frames can be in the pipeline at a given time.
    constexpr static inline size_t k_frame_depth = 3;

    // How much data we need to store in each command queue per frame.
    constexpr static inline size_t k_command_queue_size = 16'000'000;

    ri_interface& m_render_interface;
    window& m_window;
    asset_manager& m_asset_manager;

    std::vector<std::unique_ptr<render_system>> m_systems;
    std::unique_ptr<render_graph> m_render_graph;

    std::unique_ptr<render_param_block_manager> m_param_block_manager;
    std::unique_ptr<render_effect_manager> m_effect_manager;
    std::unique_ptr<render_scene_manager> m_scene_manager;

    // Command queue.

    std::array<std::unique_ptr<render_command_queue>, k_frame_depth> m_command_queues;
    size_t m_command_queue_active_index = 0;

    std::atomic_size_t m_next_render_object_id { 1 };

    // Render job dispatch management.

    std::queue<std::unique_ptr<render_world_state>> m_pending_frames;
    std::mutex m_pending_frames_mutex;
    std::condition_variable m_pending_frame_convar;

    std::atomic_size_t m_frames_in_flight { 0 };
    std::atomic_bool m_render_job_active { false };

    task_handle m_render_job_task;

    // Swapchain

    std::unique_ptr<ri_swapchain> m_swapchain;
    ri_texture* m_current_backbuffer = nullptr;

    // gbuffer

    static inline constexpr size_t k_gbuffer_count = 3;
    std::unique_ptr<ri_texture> m_depth_texture;
    std::array<std::unique_ptr<ri_texture>, k_gbuffer_count> m_gbuffer_textures;
    std::unique_ptr<ri_sampler> m_gbuffer_sampler;

    std::unique_ptr<ri_param_block> m_gbuffer_param_block;

};

}; // namespace ws
