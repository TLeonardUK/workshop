// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_texture_streamer.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/objects/render_view.h"

#include "workshop.core/perf/profile.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/memory/memory_tracker.h"

#include "workshop.render_interface/ri_interface.h"

namespace ws {

render_texture_streamer::render_texture_streamer(renderer& renderer)
    : m_renderer(renderer)
{
}

render_texture_streamer::~render_texture_streamer()
{
    if (m_async_update_task.is_valid())
    {
        m_async_update_task.wait(true);
        m_async_update_task.reset();;
    }
}

void render_texture_streamer::register_init(init_list& list)
{
}

size_t render_texture_streamer::get_resident_mip_count(texture* tex)
{
    if (auto iter = m_streaming_textures.find(tex); iter != m_streaming_textures.end())
    {
        return iter->second->resident_mips;
    }

    const render_options& options = m_renderer.get_options();
    return tex->mip_levels;// options.texture_streaming_min_resident_mips;
}

void render_texture_streamer::register_texture(texture* tex)
{
    const render_options& options = m_renderer.get_options();

    std::unique_ptr<texture_streaming_info> info = std::make_unique<texture_streaming_info>();
    info->instance = tex;
    info->resident_mips = tex->mip_levels - options.textures_dropped_mips;

    m_streaming_textures[tex] = std::move(info);
}

void render_texture_streamer::unregister_texture(texture* tex)
{
    m_streaming_textures.erase(tex);
}

void render_texture_streamer::begin_frame()
{
    m_async_update_task = async("texture streamer update", task_queue::standard, [this]() {
        async_update();
    });
}

void render_texture_streamer::end_frame()
{
    // Wait for current update task to complete.
    if (m_async_update_task.is_valid())
    {
        m_async_update_task.wait(true);
        m_async_update_task.reset();
    }

    // TODO: Apply any neccessary residency changes for textures.
}

void render_texture_streamer::calculate_in_view_mips()
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    render_visibility_manager& visible_manager = m_renderer.get_visibility_manager();

    // Gather all views that can determine streaming state.
    std::vector<render_view*> views = scene_manager.get_views();
    
    // Gather a list of all materials.
    std::unordered_set<texture*> textures;

    /*
    std::vector<render_static_mesh*> static_meshes = scene_manager.get_static_meshes();
    for (render_view* view : views)
    {
        for (render_static_mesh* mesh : static_meshes)
        {
            if (visible_manager.is_object_visibile(view->get_visibility_view_id(), mesh->get_visibility_id()))
            {
                for (auto& mat : mesh->get_materials())
                {
                    //calculate_ideal_mip_level(mat, mesh->get_bounds());
                }
            }
        }
    }*/
}

void render_texture_streamer::async_update()
{
    calculate_in_view_mips();
}

}; // namespace ws
