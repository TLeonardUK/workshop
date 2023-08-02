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
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/render_imgui_manager.h"
#include "workshop.renderer/render_command_queue.h"
#include "workshop.render_interface/ri_types.h"
#include "workshop.render_interface/ri_param_block.h"

#include "workshop.assets/asset_manager.h"

#include "workshop.debug_menu/debug_menu.h"

#include <queue>

namespace ws {

class ri_swapchain;
class ri_interface;
class ri_texture;
class ri_sampler;
class ri_param_block_archetype; 
class ri_query;
class render_view;
class render_param_block_manager;
class render_effect_manager;
class render_batch_manager;
class asset_manager;
class window;
class shader;
class input_interface;
class debug_menu;
class statistics_channel;

// Decribes the usage of a specific default texture, used to select
// an appropriate one when calling renderer::get_default_texture.
enum class default_texture_type
{
    black,      // RGBA 0,0,0,0
    white,      // RGBA 1,1,1,1
    normal,     // RGBA 0,0,1,1

    COUNT
};

// Decribes the usage of a specific default sampler, used to select
// an appropriate one when calling renderer::get_default_sampler.
enum class default_sampler_type
{
    color,      // Configured for sampling color data.
    normal,     // Configured for sampling normal data.
    shadow_map, // Configured for sampling shadow maps - clamps edges.

    COUNT
};

// Switches the resolve mode to visualization different elements
// of the render pipeline.
enum class visualization_mode
{
    normal, 
    albedo,
    wireframe,
    metallic,
    roughness,
    world_normal,
    world_position,
    lighting,
    shadow_cascades,
    light_clusters,
    light_heatmap,
    light_probes,
    light_probe_contribution,

    COUNT
};

static inline const char* visualization_mode_strings[] = {
    "normal",
    "albedo",
    "wireframe",
    "metallic",
    "roughness",
    "world normal",
    "world position",
    "lighting",
    "shadow cascades",
    "light clusters",
    "light heatmap",
    "light probes",
    "light probe contribution"
};

// Defines a set of preloaded models that are commonly used for debugging.
enum class debug_model
{
    sphere,

    COUNT
};

// If defined this adds an extra plane to the gbuffer for writing debug data to.
//#define GBUFFER_DEBUG_DATA

// ================================================================================================
//  Provides an abstracted interface for all rendering behaviour in the engine 
// ================================================================================================
class renderer
{
public:

    renderer() = delete;
    renderer(ri_interface& rhi, input_interface& input, window& main_window, asset_manager& asset_manager, debug_menu& debug_menu);

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
                return static_cast<T*>(system.get());
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

    // Gets the imgui manager.
    render_imgui_manager& get_imgui_manager();

    // Gets the param block archetype repository.
    render_param_block_manager& get_param_block_manager();

    // Gets the effect repository.
    render_effect_manager& get_effect_manager();

    // Gets the manager that handles the objects contained in the render scene.
    render_scene_manager& get_scene_manager();

    // Gets the manager that handles creating batches for instance rendering.
    render_batch_manager& get_batch_manager();    

    // Gets the current command queue that should be written to.
    // This is only valid for the current frame, do not try to cache this.
    render_command_queue& get_command_queue();

    // Gets the next opaque id of a render object. This is thread safe.
    render_object_id next_render_object_id();

    // Gets a default texture for the given usage.
    ri_texture* get_default_texture(default_texture_type type);

    // Gets a default sampler for the given usage.
    ri_sampler* get_default_sampler(default_sampler_type type);

    // Gets the current frame index being constructed.
    size_t get_frame_index();

    // Similar to get_frame_index except its used for culling and can be paused when rendering is frozen. 
    size_t get_visibility_frame_index();

    // Queues a callback that runs on the render thread.
    void queue_callback(void* source, std::function<void()> callback);

    // Unqueues all callbacks queued from the given source.
    void unqueue_callbacks(void* source);

    // Drains the renderer of all pending gpu work.
    void drain();

    // Halts the render thread and drains all gpu work. Rendering will not continue
    // until resume is called.
    void pause();

    // Resumes halted render thread caused by pause.
    void resume();

    // Gets the width/height of the output target we are rendering to.
    size_t get_display_width();
    size_t get_display_height();

    // Gets the current visualization mode on the render thread.
    visualization_mode get_visualization_mode();

    // Returns true if the rendering is frozen and culling/etc should not be updated.
    bool is_rendering_frozen();

    // Gets buffers desc
    void get_fullscreen_buffers(ri_data_layout layout, ri_buffer*& out_vertex, ri_buffer*& out_index);

    // Gets a pointer to the given debug model.
    asset_ptr<model> get_debug_model(debug_model model);

    // Returns true if we should be using direct lighting.
    bool should_draw_direct_lighting();

    // Returns true if we should be using ambient lighting.
    bool should_draw_ambient_lighting();

private:

    friend class render_command_queue;

    result<void> load_debug_models();
    result<void> unload_debug_models();

    result<void> create_resources();
    result<void> destroy_resources();

    result<void> create_systems(init_list& list);
    result<void> destroy_systems();

    result<void> create_managers(init_list& list);
    result<void> destroy_managers();

    result<void> register_asset_loaders();
    result<void> unregister_asset_loaders();

    result<void> recreate_resizable_targets();

    // Performs any needed setup for any debug menu options
    // exposed by the renderer.
    result<void> create_debug_menu();
    result<void> destroy_debug_menu();

    // Called to run one or more render jobs currently in the queue.
    void render_job();

    // Renders the given world state.
    void render_state(render_world_state& state);

    // Executes all the commands stored in the given world state.
    void process_render_commands(render_world_state& state);

    // Renders the give view.
    void render_single_view(render_world_state& state, render_view& view, std::vector<render_pass::generated_state>& output);

    // Renders all pre-view work.
    void render_pre_views(render_world_state& state, std::vector<render_pass::generated_state>& output);

    // Renders all post-view work.
    void render_post_views(render_world_state& state, std::vector<render_pass::generated_state>& output);

    // Runs all callbacks that have been queued.
    void run_callbacks();

    // Switchs the visualization mode. 
    // Don't call directly, use the command queue to change only
    // on the render thread.
    void set_visualization_mode(visualization_mode mode);

    // Draws a little debug display window with general rendering stats.
    void draw_debug_overlay();

private:

    // How many frames can be in the pipeline at a given time.
    constexpr static inline size_t k_frame_depth = 3;

    // How much data we need to store in each command queue per frame.
    constexpr static inline size_t k_command_queue_size = 32'000'000;

    ri_interface& m_render_interface;
    input_interface& m_input_interface;
    window& m_window;
    asset_manager& m_asset_manager;
    debug_menu& m_debug_menu;

    std::vector<std::unique_ptr<render_system>> m_systems;

    std::unique_ptr<render_param_block_manager> m_param_block_manager;
    std::unique_ptr<render_effect_manager> m_effect_manager;
    std::unique_ptr<render_scene_manager> m_scene_manager;
    std::unique_ptr<render_batch_manager> m_batch_manager;
    std::unique_ptr<render_imgui_manager> m_imgui_manager;

    // Debug models.

    std::array<asset_ptr<model>, static_cast<int>(debug_model::COUNT)> m_debug_models;

    // Fullscreen buffers.

    struct fullscreen_buffers
    {
        ri_data_layout layout;
        std::unique_ptr<ri_buffer> vertex_buffer;
        std::unique_ptr<ri_buffer> index_buffer;
    };

    std::mutex m_fullscreen_buffers_mutex;
    std::vector<fullscreen_buffers> m_fullscreen_buffers;

    // Debug menu.

    visualization_mode m_visualization_mode = visualization_mode::normal;
    bool m_draw_octtree_cell_bounds = false;
    bool m_draw_object_bounds = false;
    bool m_rendering_frozen = false;
    bool m_draw_debug_overlay = true;
    bool m_draw_direct_lighting = true;
    bool m_draw_ambient_lighting = true;
    size_t m_frozen_rendering_frame = 0;

    std::vector<debug_menu::option_handle> m_debug_menu_options;

    std::unique_ptr<ri_query> m_gpu_time_query;

    statistics_channel* m_stats_triangles_rendered;
    statistics_channel* m_stats_draw_calls;
    statistics_channel* m_stats_drawn_instances;
    statistics_channel* m_stats_culled_instances;
    statistics_channel* m_stats_frame_time_render;
    statistics_channel* m_stats_frame_time_game;
    statistics_channel* m_stats_frame_time_gpu;
    statistics_channel* m_stats_frame_time_render_wait;
    statistics_channel* m_stats_frame_time_present_wait;
    statistics_channel* m_stats_frame_rate;
    statistics_channel* m_stats_render_bytes_uploaded;

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

    size_t m_frame_index = 0;
    size_t m_visibility_frame_index = 0;

    bool m_paused = false;

    // Swapchain

    std::unique_ptr<ri_swapchain> m_swapchain;
    ri_texture* m_current_backbuffer = nullptr;

    // Default resources

    std::array<std::unique_ptr<ri_texture>, static_cast<int>(default_texture_type::COUNT)> m_default_textures;
    std::array<std::unique_ptr<ri_sampler>, static_cast<int>(default_sampler_type::COUNT)> m_default_samplers;

    // gbuffer
#ifdef GBUFFER_DEBUG_DATA
    static inline constexpr size_t k_gbuffer_count = 4;
#else
    static inline constexpr size_t k_gbuffer_count = 3;
#endif
    std::unique_ptr<ri_texture> m_depth_texture;
    std::array<std::unique_ptr<ri_texture>, k_gbuffer_count> m_gbuffer_textures;
    std::unique_ptr<ri_sampler> m_gbuffer_sampler;

    std::unique_ptr<ri_param_block> m_gbuffer_param_block;

    // Callbacks

    struct callback
    {
        void* source;
        std::function<void()> callback_function;
    };

    std::mutex m_callback_mutex;
    std::vector<callback> m_callbacks;

};

}; // namespace ws
