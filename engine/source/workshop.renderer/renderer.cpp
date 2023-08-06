// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/renderer.h"
#include "workshop.render_interface/ri_interface.h"

#include "workshop.renderer/systems/render_system_clear.h"
#include "workshop.renderer/systems/render_system_resolve_backbuffer.h"
#include "workshop.renderer/systems/render_system_lighting.h"
#include "workshop.renderer/systems/render_system_imgui.h"
#include "workshop.renderer/systems/render_system_geometry.h"
#include "workshop.renderer/systems/render_system_transparent_geometry.h"
#include "workshop.renderer/systems/render_system_debug.h"
#include "workshop.renderer/systems/render_system_shadows.h"
#include "workshop.renderer/systems/render_system_ssao.h"
#include "workshop.renderer/systems/render_system_light_probes.h"
#include "workshop.renderer/systems/render_system_reflection_probes.h"
#include "workshop.renderer/render_graph.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_scene_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/render_imgui_manager.h"
#include "workshop.renderer/render_command_queue.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/assets/shader/shader.h"
#include "workshop.renderer/assets/shader/shader_loader.h"
#include "workshop.renderer/assets/model/model_loader.h"
#include "workshop.renderer/assets/texture/texture_loader.h"
#include "workshop.renderer/assets/material/material_loader.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_swapchain.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_sampler.h"
#include "workshop.render_interface/ri_param_block.h"
#include "workshop.render_interface/ri_query.h"

#include "workshop.debug_menu/debug_menu.h"

#include "workshop.assets/asset_manager.h"

#include "workshop.window_interface/window.h"

#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/async/async.h"
#include "workshop.core/perf/profile.h"
#include "workshop.core/perf/timer.h"
#include "workshop.core/hashing/hash.h"
#include "workshop.core/containers/command_queue.h"
#include "workshop.core/drawing/pixmap.h"
#include "workshop.core/statistics/statistics_manager.h"
#include "workshop.core/utils/time.h"
#include "workshop.core/platform/platform.h"

namespace ws {

renderer::renderer(ri_interface& rhi, input_interface& input, window& main_window, asset_manager& asset_manager, debug_menu& debug_menu)
    : m_render_interface(rhi)
    , m_input_interface(input)
    , m_window(main_window)
    , m_asset_manager(asset_manager)
    , m_debug_menu(debug_menu)
{
    for (size_t i = 0; i < k_frame_depth; i++)
    {
        m_command_queues[i] = std::make_unique<render_command_queue>(*this, k_command_queue_size);
    }
}

void renderer::register_init(init_list& list)
{
    list.add_step(
        "Renderer Asset Loaders",
        [this, &list]() -> result<void> { return register_asset_loaders(); },
        [this, &list]() -> result<void> { return unregister_asset_loaders(); }
    );

    list.add_step(
        "Renderer Managers",
        [this, &list]() -> result<void> { return create_managers(list); },
        [this, &list]() -> result<void> { return destroy_managers(); }
    );

    list.add_step(
        "Renderer Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );

    list.add_step(
        "Renderer Systems",
        [this, &list]() -> result<void> { return create_systems(list); },
        [this, &list]() -> result<void> { return destroy_systems(); }
    );

    list.add_step(
        "Renderer Debug Menu",
        [this, &list]() -> result<void> { return create_debug_menu(); },
        [this, &list]() -> result<void> { return destroy_debug_menu(); }
    );
    list.add_step(
        "Renderer Debug Models",
        [this, &list]() -> result<void> { return load_debug_models(); },
        [this, &list]() -> result<void> { return unload_debug_models(); }
    );
}

result<void> renderer::create_systems(init_list& list)
{
    // Note: Order is important here, this is the order the 
    //       stages will be added to the render graph in.
    m_systems.push_back(std::make_unique<render_system_clear>(*this));
    m_systems.push_back(std::make_unique<render_system_geometry>(*this));
    m_systems.push_back(std::make_unique<render_system_shadows>(*this));
    m_systems.push_back(std::make_unique<render_system_ssao>(*this));
    m_systems.push_back(std::make_unique<render_system_lighting>(*this));
    m_systems.push_back(std::make_unique<render_system_transparent_geometry>(*this));
    m_systems.push_back(std::make_unique<render_system_light_probes>(*this, m_debug_menu));
    m_systems.push_back(std::make_unique<render_system_reflection_probes>(*this, m_debug_menu));
    m_systems.push_back(std::make_unique<render_system_resolve_backbuffer>(*this));
    m_systems.push_back(std::make_unique<render_system_debug>(*this));
    m_systems.push_back(std::make_unique<render_system_imgui>(*this));

    for (auto& system : m_systems)
    {
        system->register_init(list);
    }

    return true;
}

result<void> renderer::destroy_systems()
{
    m_systems.clear();

    return true;
}

result<void> renderer::create_managers(init_list& list)
{
    m_effect_manager = std::make_unique<render_effect_manager>(*this, m_asset_manager);
    m_param_block_manager = std::make_unique<render_param_block_manager>(*this);
    m_scene_manager = std::make_unique<render_scene_manager>(*this);
    m_visibility_manager = std::make_unique<render_visibility_manager>(*this);
    m_batch_manager = std::make_unique<render_batch_manager>(*this);
    m_imgui_manager = std::make_unique<render_imgui_manager>(*this, m_input_interface);

    m_param_block_manager->register_init(list);
    m_effect_manager->register_init(list);
    m_scene_manager->register_init(list);
    m_visibility_manager->register_init(list);
    m_batch_manager->register_init(list);
    m_imgui_manager->register_init(list);

    return true;
}

result<void> renderer::destroy_managers()
{
    m_effect_manager = nullptr;
    m_param_block_manager = nullptr;
    m_scene_manager = nullptr;
    m_visibility_manager = nullptr;
    m_batch_manager = nullptr;
    m_imgui_manager = nullptr;

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

    // Create a query for monitoring gpu time.
    ri_query::create_params params;
    params.type = ri_query_type::time;
    m_gpu_time_query = m_render_interface.create_query(params, "Gpu time query");

    return true;
}

result<void> renderer::destroy_resources()
{
    // Ensure all render jobs have completed.
    if (m_render_job_task.is_valid())
    {
        m_render_job_task.wait();
    }

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


result<void> renderer::load_debug_models()
{
    m_debug_models[(int)debug_model::sphere] = m_asset_manager.request_asset<model>("data:models/debug/sphere.yaml", 0);

    for (size_t i = 0; i < m_debug_models.size(); i++)
    {
        m_debug_models[i].wait_for_load();

        if (!m_debug_models[i].is_loaded())
        {
            db_error(renderer, "Failed to load debug model: %s", m_debug_models[i].get_path().c_str());
            return false;
        }
    }

    return true;
}

result<void> renderer::unload_debug_models()
{
    for (size_t i = 0; i < m_debug_models.size(); i++)
    {
        m_debug_models[i] = {};
    }

    return true;
}

result<void> renderer::register_asset_loaders()
{
    m_asset_manager.register_loader(std::make_unique<shader_loader>(get_render_interface(), *this));
    m_asset_manager.register_loader(std::make_unique<model_loader>(get_render_interface(), *this, m_asset_manager));
    m_asset_manager.register_loader(std::make_unique<material_loader>(get_render_interface(), *this, m_asset_manager));
    m_asset_manager.register_loader(std::make_unique<texture_loader>(get_render_interface(), *this));

    return true;
}

result<void> renderer::unregister_asset_loaders()
{
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
    params.format = ri_texture_format::R16G16B16A16_FLOAT;
    m_gbuffer_textures[0] = m_render_interface.create_texture(params, "gbuffer[0] rgb:diffuse a:flags");

    params.format = ri_texture_format::R16G16B16A16_FLOAT;
    m_gbuffer_textures[1] = m_render_interface.create_texture(params, "gbuffer[1] rgb:world normal a:roughness");

    params.format = ri_texture_format::R32G32B32A32_FLOAT;
    m_gbuffer_textures[2] = m_render_interface.create_texture(params, "gbuffer[2] rgb:world position a:metallic");
#ifdef GBUFFER_DEBUG_DATA
    m_gbuffer_textures[3] = m_render_interface.create_texture(params, "gbuffer[3] rgba:debug data");
#endif

    // Create sampler used for reading the gbuffer.
    ri_sampler::create_params sampler_params;
    m_gbuffer_sampler = m_render_interface.create_sampler(sampler_params, "gbuffer sampler");

    // Create param block describing the above.
    m_gbuffer_param_block = m_param_block_manager->create_param_block("gbuffer");
    m_gbuffer_param_block->set("gbuffer0_texture", *m_gbuffer_textures[0]);
    m_gbuffer_param_block->set("gbuffer1_texture", *m_gbuffer_textures[1]);
    m_gbuffer_param_block->set("gbuffer2_texture", *m_gbuffer_textures[2]);
#ifdef GBUFFER_DEBUG_DATA
    m_gbuffer_param_block->set("gbuffer3_texture", *m_gbuffer_textures[3]);
#endif
    m_gbuffer_param_block->set("gbuffer_sampler", *m_gbuffer_sampler);

    // Create default resources.
    std::vector<uint8_t> default_data = { 0, 0, 0, 0 };
    params.format = ri_texture_format::R8G8B8A8;
    params.width = 1;
    params.height = 1;
    params.data = default_data;
    params.is_render_target = false;

    default_data[0] = 0;
    default_data[1] = 0;
    default_data[2] = 0;
    default_data[3] = 0;
    m_default_textures[static_cast<int>(default_texture_type::black)] = m_render_interface.create_texture(params, "default black texture");

    default_data[0] = 255;
    default_data[1] = 255;
    default_data[2] = 255;
    default_data[3] = 255;
    m_default_textures[static_cast<int>(default_texture_type::white)] = m_render_interface.create_texture(params, "default white texture");

    default_data[0] = 128;
    default_data[1] = 128;
    default_data[2] = 255;
    default_data[3] = 255;
    m_default_textures[static_cast<int>(default_texture_type::normal)] = m_render_interface.create_texture(params, "default normal texture");

    default_data[0] = 128;
    default_data[1] = 128;
    default_data[2] = 128;
    default_data[3] = 128;
    m_default_textures[static_cast<int>(default_texture_type::grey)] = m_render_interface.create_texture(params, "default grey texture");

    m_default_samplers[static_cast<int>(default_sampler_type::color)] = m_render_interface.create_sampler(sampler_params,  "default color sampler");
    m_default_samplers[static_cast<int>(default_sampler_type::normal)] = m_render_interface.create_sampler(sampler_params,  "default normal sampler");

    sampler_params.address_mode_u = ri_texture_address_mode::clamp_to_edge;
    sampler_params.address_mode_v = ri_texture_address_mode::clamp_to_edge;
    sampler_params.address_mode_w = ri_texture_address_mode::clamp_to_edge;
    sampler_params.filter = ri_texture_filter::linear;
    sampler_params.border_color = ri_texture_border_color::opaque_white;
    m_default_samplers[static_cast<int>(default_sampler_type::shadow_map)] = m_render_interface.create_sampler(sampler_params, "default shadow map sampler");

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
#ifdef GBUFFER_DEBUG_DATA
    output.color_targets.push_back(m_gbuffer_textures[3].get());
#endif
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

render_visibility_manager& renderer::get_visibility_manager()
{
    return *m_visibility_manager;
}

render_batch_manager& renderer::get_batch_manager()
{
    return *m_batch_manager;
}

render_imgui_manager& renderer::get_imgui_manager()
{
    return *m_imgui_manager;
}

render_command_queue& renderer::get_command_queue()
{
    return *m_command_queues[m_command_queue_active_index];
}

ri_texture* renderer::get_default_texture(default_texture_type type)
{
    return m_default_textures[static_cast<int>(type)].get();
}

ri_sampler* renderer::get_default_sampler(default_sampler_type type)
{
    return m_default_samplers[static_cast<int>(type)].get();
}

size_t renderer::get_frame_index()
{
    return m_frame_index;
}

render_object_id renderer::next_render_object_id()
{
    return static_cast<render_object_id>(m_next_render_object_id.fetch_add(1));
}

void renderer::queue_callback(void* source, std::function<void()> callback)
{
    std::scoped_lock lock(m_callback_mutex);

    m_callbacks.push_back({ source, callback });
}

void renderer::unqueue_callbacks(void* source)
{
    std::scoped_lock lock(m_callback_mutex);

    auto start_iter = std::remove_if(m_callbacks.begin(), m_callbacks.end(), [source](const callback& instance) {
        return source == instance.source;
    });
    m_callbacks.erase(start_iter, m_callbacks.end());
}

void renderer::run_callbacks()
{
    std::scoped_lock lock(m_callback_mutex);

    profile_marker(profile_colors::render, "process render callbacks");

    for (callback& instance : m_callbacks)
    {
        instance.callback_function();
    }

    m_callbacks.clear();
}

void renderer::process_render_commands(render_world_state& state)
{
    profile_marker(profile_colors::render, "process render commands");

    render_command_queue& queue = *state.command_queue;

    while (!queue.empty())
    {
        queue.execute_next();
    }

    queue.reset();
}

void renderer::render_state(render_world_state& state)
{
    ri_command_queue& graphics_command_queue = m_render_interface.get_graphics_queue();
    ri_command_queue& copy_command_queue = m_render_interface.get_copy_queue();

    profile_marker(profile_colors::render, "frame %zi", (size_t)state.time.frame_count);

    timer frame_timer;
    frame_timer.start();

    m_frame_index = state.time.frame_count;

    // Debug drawing.
    {
        profile_marker(profile_colors::render, "debug drawing");

        if (m_draw_octtree_cell_bounds || m_draw_object_bounds)
        {
            m_visibility_manager->draw_cell_bounds(m_draw_octtree_cell_bounds, m_draw_object_bounds);
        }

        if (m_draw_debug_overlay)
        {
            draw_debug_overlay();
        }
    }

    // Grab the next backbuffer, and wait for gpu if pipeline is full.
    {
        profile_marker(profile_colors::render, "get next back buffer");

        timer wait_timer;
        wait_timer.start();

        m_current_backbuffer = &get_next_backbuffer();

        wait_timer.stop();
        m_stats_frame_time_present_wait->submit(wait_timer.get_elapsed_seconds());
    }

    // Execute any callbacks that have been queued.
    run_callbacks();

    // Process the command queue.
    process_render_commands(state);

    // Update all systems in parallel.
    parallel_for("step render systems", task_queue::standard, m_systems.size(), [this, &state](size_t index) {
        profile_marker(profile_colors::render, "step render system: %s", m_systems[index]->name.c_str());
        m_systems[index]->step(state);
    });

    // Begin the new frame.
    m_render_interface.begin_frame();
    m_batch_manager->begin_frame();

    // Perform frustum culling for all views.
    if (!m_rendering_frozen)
    {
        m_visibility_manager->update_visibility();
    }

    // Render each view.
    std::vector<render_view*> views = m_scene_manager->get_views();

    // Strip out any views not markled for rendering this frame.
    auto iter = std::remove_if(views.begin(), views.end(), [](render_view* view) { return !view->should_render(); });
    if (iter != views.end())
    {
        views.erase(iter, views.end());
    }

    struct view_generated_state
    {
        std::vector<render_pass::generated_state> states;
        render_view* view;
    };
    std::vector<view_generated_state> view_generated_states;
    view_generated_states.resize(views.size());

    parallel_for("render views", task_queue::standard, views.size(), [this, &state, &views, &view_generated_states](size_t index) {
        profile_marker(profile_colors::render, "render view: %s", views[index]->get_name().c_str());

        view_generated_state& view_state = view_generated_states[index];
        view_state.view = views[index];

        render_single_view(state, *views[index], view_state.states);
    });

    // Sort generated states by view render order.
    std::sort(view_generated_states.begin(), view_generated_states.end(), [](const view_generated_state& a, const view_generated_state& b) {
        return a.view->get_view_order() < b.view->get_view_order();
    });

    // Do pre generation.
    std::vector<render_pass::generated_state> pre_generated_states;
    render_pre_views(state, pre_generated_states);

    // Do post generation.
    std::vector<render_pass::generated_state> post_generated_states;
    render_post_views(state, post_generated_states);

    // Before dispatching command lists flush any uploads that may have been queued as part of generation.
    m_render_interface.flush_uploads();

    // Start querying gpu timer.
    {
        profile_marker(profile_colors::render, "start gpu timer");

        if (m_gpu_time_query->are_results_ready())
        {
            double gpu_time = m_gpu_time_query->get_results();
            m_stats_frame_time_gpu->submit(gpu_time);
        }

        ri_command_list& list = graphics_command_queue.alloc_command_list();
        list.open();
        list.begin_query(m_gpu_time_query.get());
        list.close();
        graphics_command_queue.execute(list);
    }

    // Dispatch all generated command lists.
    {
        profile_marker(profile_colors::render, "dispatch command lists");

        // Dispatch all pre work.
        {
            profile_gpu_marker(graphics_command_queue, profile_colors::gpu_view, "pre views");

            for (render_pass::generated_state& state : pre_generated_states)
            {
                for (auto& graphics_list : state.graphics_command_lists)
                {
                    graphics_command_queue.execute(*graphics_list);
                }
            }
        }

        // Dispatch all generated states for each view.
        for (size_t i = 0; i < view_generated_states.size(); i++)
        {            
            view_generated_state& generated_state_list = view_generated_states[i];

            profile_gpu_marker(graphics_command_queue, profile_colors::gpu_view, "render view: %s", generated_state_list.view->get_name().c_str());

            for (render_pass::generated_state& state : generated_state_list.states)
            {
                for (auto& graphics_list : state.graphics_command_lists)
                {
                    graphics_command_queue.execute(*graphics_list);
                }
            }
        }

        // Dispatch all post work.
        {
            profile_gpu_marker(graphics_command_queue, profile_colors::gpu_view, "post views");

            for (render_pass::generated_state& state : post_generated_states)
            {
                for (auto& graphics_list : state.graphics_command_lists)
                {
                    graphics_command_queue.execute(*graphics_list);
                }
            }
        }
    }

    // Stop querying gpu timer.
    {
        profile_marker(profile_colors::render, "stop gpu timer");

        ri_command_list& list = graphics_command_queue.alloc_command_list();
        list.open();
        list.end_query(m_gpu_time_query.get());
        list.close();
        graphics_command_queue.execute(list);
    }

    // Present, we're done with this frame!
    {
        profile_marker(profile_colors::render, "present");

        timer wait_timer;
        wait_timer.start();

        m_swapchain->present();

        wait_timer.stop();
        m_stats_frame_time_present_wait->submit(wait_timer.get_elapsed_seconds());
    }

    // End the frame.
    m_render_interface.end_frame();

    // Store render time.
    frame_timer.stop();
    m_stats_frame_time_render->submit(frame_timer.get_elapsed_seconds());

    // Commit rendering statistics.
    statistics_manager::get().commit(statistics_commit_point::end_of_render);
}

void renderer::render_single_view(render_world_state& state, render_view& view, std::vector<render_pass::generated_state>& output)
{
    profile_marker(profile_colors::render, "render view");

    // Generate a render graph for rendering this graph.
    render_graph graph;
    {
        profile_marker(profile_colors::render, "build graph");

        for (auto& system : m_systems)
        {
            system->build_graph(graph, state, view);
        }
    }

    std::vector<render_graph::node*> nodes;
    {
        profile_marker(profile_colors::render, "get active nodes");

        graph.get_active(nodes);
        output.resize(nodes.size());
    }
 
    // Render each pass in parallel.
    parallel_for("generate render passs", task_queue::standard, nodes.size(), [this, &state, &output, &nodes, &view](size_t index) {
        render_graph::node* node = nodes[index];
        
        profile_marker(profile_colors::render, "generate render pass: %s", node->pass->name.c_str());        
        //get_technique(node->pass->effect_name.c_str(), node->pass->effect_variations);

        node->pass->generate(*this, output[index], &view);
    });
}

void renderer::render_pre_views(render_world_state& state, std::vector<render_pass::generated_state>& output)
{
    profile_marker(profile_colors::render, "render pre view");

    // Generate a render graph for rendering this graph.
    render_graph graph;
    {
        profile_marker(profile_colors::render, "build graph");

        for (auto& system : m_systems)
        {
            system->build_pre_graph(graph, state);
        }
    }

    std::vector<render_graph::node*> nodes;
    {
        profile_marker(profile_colors::render, "get active nodes");

        graph.get_active(nodes);
        output.resize(nodes.size());
    }
 
    // Render each pass in parallel.
    parallel_for("generate render passs", task_queue::standard, nodes.size(), [this, &state, &output, &nodes](size_t index) {
        render_graph::node* node = nodes[index];
        
        profile_marker(profile_colors::render, "generate render pass: %s", node->pass->name.c_str());        

        node->pass->generate(*this, output[index], nullptr);
    });
}

void renderer::render_post_views(render_world_state& state, std::vector<render_pass::generated_state>& output)
{
    profile_marker(profile_colors::render, "render post view");

    // Generate a render graph for rendering this graph.
    render_graph graph;
    {
        profile_marker(profile_colors::render, "build graph");

        for (auto& system : m_systems)
        {
            system->build_post_graph(graph, state);
        }
    }

    std::vector<render_graph::node*> nodes;
    {
        profile_marker(profile_colors::render, "get active nodes");

        graph.get_active(nodes);
        output.resize(nodes.size());
    }
 
    // Render each pass in parallel.
    parallel_for("generate render passs", task_queue::standard, nodes.size(), [this, &state, &output, &nodes](size_t index) {
        render_graph::node* node = nodes[index];
        
        profile_marker(profile_colors::render, "generate render pass: %s", node->pass->name.c_str());        

        node->pass->generate(*this, output[index], nullptr);
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
    // Step all sub-managers.
    m_imgui_manager->step(state->time);

    // Push this frame into the queue, and optionally wait if 
    // queue depth has gotten too great.
    bool start_new_render_job = false;
    {
        std::unique_lock lock(m_pending_frames_mutex);
        m_pending_frames.push(std::move(state));
        m_pending_frames.back()->command_queue = m_command_queues[m_command_queue_active_index].get();
        m_frames_in_flight.fetch_add(1);

        timer wait_timer;
        wait_timer.start();

        // Wait for previous frames to complete if depth is high enough.
        while (m_frames_in_flight.load() >= k_frame_depth)
        {
            profile_marker(profile_colors::render, "wait for render");
            m_pending_frame_convar.wait(lock);
        }

        wait_timer.stop();
        m_stats_frame_time_render_wait->submit(wait_timer.get_elapsed_seconds());

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

    // Cycle the command queue to write to.
    m_command_queue_active_index = (m_command_queue_active_index + 1) % m_command_queues.size();
}

void renderer::pause()
{
    std::unique_lock lock(m_pending_frames_mutex);

    while (m_frames_in_flight.load() > 0)
    {
        profile_marker(profile_colors::render, "wait for render");
        m_pending_frame_convar.wait(lock);
    }

    drain();

    m_paused = true;
}
 
void renderer::resume()
{
    m_paused = false;
}

result<void> renderer::create_debug_menu()
{
    // Grab statistics the debug menu will want.
    m_stats_triangles_rendered = statistics_manager::get().find_or_create_channel("rendering/triangles_rendered");
    m_stats_draw_calls = statistics_manager::get().find_or_create_channel("rendering/draw_calls");
    m_stats_drawn_instances = statistics_manager::get().find_or_create_channel("rendering/drawn_instances");
    m_stats_culled_instances = statistics_manager::get().find_or_create_channel("rendering/culled_instances");
    m_stats_frame_time_render = statistics_manager::get().find_or_create_channel("frame time/render", 1.0, statistics_commit_point::end_of_render);
    m_stats_frame_time_render_wait = statistics_manager::get().find_or_create_channel("frame time/render wait", 1.0, statistics_commit_point::end_of_game);
    m_stats_frame_time_present_wait = statistics_manager::get().find_or_create_channel("frame time/present wait", 1.0, statistics_commit_point::end_of_render);
    m_stats_frame_time_game = statistics_manager::get().find_or_create_channel("frame time/game", 1.0, statistics_commit_point::end_of_game);
    m_stats_frame_time_gpu = statistics_manager::get().find_or_create_channel("frame time/gpu", 1.0, statistics_commit_point::end_of_render);
    m_stats_frame_rate = statistics_manager::get().find_or_create_channel("frame rate");
    m_stats_render_bytes_uploaded = statistics_manager::get().find_or_create_channel("render/bytes uploaded");

    // Setup debug menu toggles.
    for (size_t i = 0; i < static_cast<size_t>(visualization_mode::COUNT); i++)
    {
        std::string path = string_format("rendering/visualization/%s", visualization_mode_strings[i]);

        auto option = m_debug_menu.add_option(path.c_str(), [this, i]() {
            get_command_queue().set_visualization_mode(static_cast<visualization_mode>(i));
        });

        m_debug_menu_options.push_back(std::move(option));
    }

    m_debug_menu_options.push_back(m_debug_menu.add_option("rendering/partitioning/toggle cell bounds", [this]() { m_draw_octtree_cell_bounds = !m_draw_octtree_cell_bounds; }));
    m_debug_menu_options.push_back(m_debug_menu.add_option("rendering/partitioning/toggle object bounds", [this]() { m_draw_object_bounds = !m_draw_object_bounds; }));
    m_debug_menu_options.push_back(m_debug_menu.add_option("rendering/toggles/toggle direct lighting", [this]() { m_draw_direct_lighting = !m_draw_direct_lighting; }));
    m_debug_menu_options.push_back(m_debug_menu.add_option("rendering/toggles/toggle ambient lighting", [this]() { m_draw_ambient_lighting = !m_draw_ambient_lighting; }));
    m_debug_menu_options.push_back(m_debug_menu.add_option("rendering/toggle freeze rendering", [this]() { m_rendering_frozen = !m_rendering_frozen; }));
    m_debug_menu_options.push_back(m_debug_menu.add_option("rendering/toggle overlay", [this]() { m_draw_debug_overlay = !m_draw_debug_overlay; }));

    return true;
}

result<void> renderer::destroy_debug_menu()
{
    m_debug_menu_options.clear();

    return true;
}

void renderer::set_visualization_mode(visualization_mode mode)
{
    db_log(renderer, "Changed visualization mode to %zi", mode);
    m_visualization_mode = mode;
}

visualization_mode renderer::get_visualization_mode()
{
    return m_visualization_mode;
}

bool renderer::is_rendering_frozen()
{
    return m_rendering_frozen;
}

bool renderer::should_draw_direct_lighting()
{
    return m_draw_direct_lighting;
}

bool renderer::should_draw_ambient_lighting()
{
    return m_draw_ambient_lighting;
}

void renderer::get_fullscreen_buffers(ri_data_layout layout, ri_buffer*& out_vertex, ri_buffer*& out_index)
{
    std::scoped_lock lock(m_fullscreen_buffers_mutex);

    auto iter = std::find_if(m_fullscreen_buffers.begin(), m_fullscreen_buffers.end(), [&layout](auto& buffers) {
        return buffers.layout == layout;
    });

    if (iter != m_fullscreen_buffers.end())
    {
        out_vertex = iter->vertex_buffer.get();
        out_index = iter->index_buffer.get();
        return;
    }

    std::unique_ptr<ri_layout_factory> factory = m_render_interface.create_layout_factory(layout, ri_layout_usage::buffer);
    factory->add("position", { vector2(-1.0f, -1.0f), vector2(1.0f, -1.0f), vector2(-1.0f,  1.0f), vector2(1.0f,  1.0f) });
    factory->add("uv", { vector2(0.0f,  1.0f), vector2(1.0f,  1.0f), vector2(0.0f,  0.0f), vector2(1.0f,  0.0f) });

    fullscreen_buffers& buffers = m_fullscreen_buffers.emplace_back();
    buffers.layout = layout;
    buffers.vertex_buffer = factory->create_vertex_buffer("Fullscreen Vertex Buffer");
    buffers.index_buffer = factory->create_index_buffer("Fullscreen Index Buffer", std::vector<uint16_t> { 2, 1, 0, 3, 1, 2 });
    
    out_vertex = buffers.vertex_buffer.get();
    out_index = buffers.index_buffer.get();
}

asset_ptr<model> renderer::get_debug_model(debug_model model)
{
    return m_debug_models[(int)model];
}

void renderer::drain()
{
    if (m_swapchain)
    {
        m_swapchain->drain();
    }
}

size_t renderer::get_display_width()
{
    return m_window.get_width();
}

size_t renderer::get_display_height()
{
    return m_window.get_height();
}

void renderer::draw_debug_overlay()
{
    imgui_scope scope(get_imgui_manager(), "Rendering Debug Overlay");

    const size_t k_width = 250;
    const size_t k_padding = 30;

    size_t display_width = get_display_width();
    size_t display_height = get_display_height();

    ImGui::SetNextWindowPos(ImVec2(display_width - k_width - k_padding, k_padding), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(k_width, 0), ImGuiCond_Always);
    if (ImGui::Begin("Rendering Debug Overlay", &m_draw_debug_overlay, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::BeginTable("Stats Table", 2);

        float render_wait = m_stats_frame_time_render_wait->current_value() * 1000.0f;
        float present_wait = m_stats_frame_time_present_wait->current_value() * 1000.0f;

        float game_time = m_stats_frame_time_game->current_value() * 1000.0f;
        float render_time = m_stats_frame_time_render->current_value() * 1000.0f;
        float gpu_time = m_stats_frame_time_gpu->current_value() * 1000.0f;

        float frame_rate = m_stats_frame_rate->average_value();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("FPS");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f", frame_rate);

        ImGui::NewLine();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Game Time");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f ms", game_time - render_wait);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Render Time");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f ms", render_time - present_wait);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("GPU Time");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f ms", gpu_time);

        ImGui::NewLine();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Virtual Memory");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f mb", get_memory_usage() / (1024.0 * 1024.0f));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Pagefile Memory");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f mb", get_pagefile_usage() / (1024.0 * 1024.0f));

        size_t vram_local = 0;
        size_t vram_non_local = 0;
        m_render_interface.get_vram_usage(vram_local, vram_non_local);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("VRAM Memory (Device)");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f mb", vram_local / (1024.0 * 1024.0f));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("VRAM Memory (Shared)");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.2f mb", vram_non_local / (1024.0 * 1024.0f));

        ImGui::NewLine();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Triangles Rendered");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zi", static_cast<size_t>(m_stats_triangles_rendered->current_value()));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Draw Calls");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zi", static_cast<size_t>(m_stats_draw_calls->current_value()));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Instances Rendered");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zi", static_cast<size_t>(m_stats_drawn_instances->current_value()));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Instances Culled");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zi", static_cast<size_t>(m_stats_culled_instances->current_value()));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Bytes Uploaded");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zi", static_cast<size_t>(m_stats_render_bytes_uploaded->current_value()));

        ImGui::EndTable();
        ImGui::End();
    }
}

}; // namespace ws
