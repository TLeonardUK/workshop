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
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_output.h"
#include "workshop.render_interface/ri_types.h"

#include "workshop.assets/asset_manager.h"

#include <queue>

namespace ws {

class ri_swapchain;
class ri_interface;
class ri_texture;
class ri_sampler;
class ri_param_block_archetype;
class render_view;
class asset_manager;
class window;
class shader;

// ================================================================================================
//  Provides an abstracted interface for all rendering behaviour in the engine 
// ================================================================================================
class renderer
{
public:

    // Identifies an effect type that has been registered to the renderer. 
    using effect_id = size_t;

    // Identifies a parameter block archetype that has been registered to the renderer.
    using param_block_archetype_id = size_t;

    // Invalid id value for any of the above.
    constexpr static inline size_t invalid_id = 0;

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

    // Gets render output for drawing to the gbuffer.
    render_output get_gbuffer_output();

    // TODO: Move all this to an effect manager.

    // Registers an effect for use by the renderer. Ownership is transfered to renderer.
    effect_id register_effect(std::unique_ptr<render_effect>&& effect);

    // Unregisters a previous registered effect. This may not take immediately effect
    // if parts of the renderer are still using the effect.
    void unregister_effect(effect_id id);

    // Gets an effect from its id.
    render_effect* get_effect(effect_id id);

    // Gets a renderer technique by its name and a set of parameter values.
    render_effect::technique* get_technique(const char* name, const std::unordered_map<std::string, std::string>& parameters);

    // TODO: Move all this to a param block manager. 

    // Registers a param block archetype that may be used for renderering. If
    // a param block with the same layout/scope is available it will be returned.
    param_block_archetype_id register_param_block_archetype(const char* name, ri_data_scope scope, const ri_data_layout& layout);

    // Unregisters a previous param block archetype. This may not take immediately effect
    // if parts of the renderer are still using the param block archetype.
    void unregister_param_block_archetype(param_block_archetype_id id);

    // Gets a param block archetype from its id.
    ri_param_block_archetype* get_param_block_archetype(param_block_archetype_id id);

private:

    result<void> create_resources();
    result<void> destroy_resources();

    result<void> create_shaders();
    result<void> destroy_shaders();

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

    // Renders the give view.
    void render_single_view(render_world_state& state, render_view& view, std::vector<render_pass::generated_state>& output);

private:

    struct param_block_state
    {
        std::string name;
        size_t register_count = 0;
        size_t hash = 0;
        std::unique_ptr<ri_param_block_archetype> instance;
    };

private:

    constexpr static inline size_t k_frame_depth = 3;

    ri_interface& m_render_interface;
    window& m_window;
    asset_manager& m_asset_manager;

    std::vector<std::unique_ptr<render_system>> m_systems;
    std::unique_ptr<render_graph> m_render_graph;

    // Render job dispatch management.

    std::queue<std::unique_ptr<render_world_state>> m_pending_frames;
    std::mutex m_pending_frames_mutex;
    std::condition_variable m_pending_frame_convar;

    std::atomic_size_t m_frames_in_flight { 0 };
    std::atomic_bool m_render_job_active { false };

    task_handle m_render_job_task;

    // Shader / effect management.

    size_t m_id_counter = 1;
    std::unordered_map<effect_id, std::unique_ptr<render_effect>> m_effects;
    std::unordered_map<param_block_archetype_id, param_block_state> m_param_block_archetypes;
    std::vector<asset_ptr<shader>> m_shader_assets;

    // Render interface resources.

    std::unique_ptr<ri_swapchain> m_swapchain;

    static inline constexpr size_t k_gbuffer_count = 3;
    std::unique_ptr<ri_texture> m_depth_texture;
    std::array<std::unique_ptr<ri_texture>, k_gbuffer_count> m_gbuffer_textures;
    std::unique_ptr<ri_sampler> m_gbuffer_sampler;

};

}; // namespace ws
