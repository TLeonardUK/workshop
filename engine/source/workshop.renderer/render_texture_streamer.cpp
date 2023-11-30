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

#pragma optimize("", off)

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

size_t render_texture_streamer::get_wanted_resident_mip_count(texture* tex)
{
    if (auto iter = m_streaming_textures.find(tex); iter != m_streaming_textures.end())
    {
        return iter->second->wanted_resident_mips;
    }

    const render_options& options = m_renderer.get_options();
    return options.texture_streaming_min_resident_mips;
}

size_t render_texture_streamer::get_ideal_resident_mip_count(texture* tex)
{
    if (auto iter = m_streaming_textures.find(tex); iter != m_streaming_textures.end())
    {
        return iter->second->ideal_resident_mips;
    }

    const render_options& options = m_renderer.get_options();
    return options.texture_streaming_min_resident_mips;
}

void render_texture_streamer::register_texture(texture* tex)
{
    // TODO: Might want to defer this so streaming_textures is not modified while calculating ideal mips.

    const render_options& options = m_renderer.get_options();

    std::unique_ptr<texture_streaming_info> info = std::make_unique<texture_streaming_info>();
    info->instance = tex;
    info->wanted_resident_mips = tex->ri_instance->get_mip_levels();
    info->ideal_resident_mips = info->wanted_resident_mips;

    m_streaming_textures[tex] = std::move(info);
}

void render_texture_streamer::unregister_texture(texture* tex)
{
    // TODO: Might want to defer this so streaming_textures is not modified while calculating ideal mips.

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

void render_texture_streamer::set_ideal_resident_mip_count(texture_streaming_info& streaming_info, size_t new_ideal_mips)
{
    if (streaming_info.ideal_resident_mips == new_ideal_mips)
    {
        return;
    }

    db_log(renderer, "[Streaming] Ideal mip changed to %zi for %s", new_ideal_mips, streaming_info.instance->name.c_str());
    streaming_info.ideal_resident_mips = new_ideal_mips;
}

size_t render_texture_streamer::calculate_ideal_mip_count(const texture_bounds& tex_bounds)
{
    const render_options& options = m_renderer.get_options();

    // Calculations here are based roughly on this:
    // http://web.cse.ohio-state.edu/~crawfis.3/cse781/Readings/MipMapLevels-Blog.html

    // Find closest point on bounds to camera.
    vector3 view_location = tex_bounds.view->get_local_location();
    vector3 closet_point = tex_bounds.bounds.get_closest_point(view_location);

    // Find the screen space area.
    float p = (float)tex_bounds.view->get_viewport().width;
    float d = (closet_point - view_location).length() + 0.001f; // small epsilon to avoid issues when inside meshes.
    float f = tex_bounds.view->get_fov();
    float q = std::powf((p / (d * std::tanf(f))), 2);

    // Determine how many mips to drop based on the texel density of the mesh.
    float world_scale = std::sqrt(tex_bounds.bounds.transform.extract_scale().max_component()); // TODO: Fix this up it gives us some wierd results with the mips to drop going negative.
    float factor = (tex_bounds.min_texel_area * tex_bounds.texture->width) / (tex_bounds.max_world_area * world_scale);
    float mips_to_drop = 0.5f * log2(factor / q);

    // Calculate the ideal mip count.
    size_t mip_count = tex_bounds.texture->ri_instance->get_mip_levels();

    // We always add 1 to the ideal mip count for trilinear filtering.
    size_t ideal_mip_count = (size_t)std::clamp(((int)mip_count - (int)std::truncf(mips_to_drop)), 0, (int)mip_count); 

    //db_log(core, "q=%.2f factor=%.2f ideal=%zi", q, mips_to_drop, ideal_mip_count);

    // Clamp to minimum and maximum mip bounds.
    ideal_mip_count = std::clamp(
        ideal_mip_count,
        options.texture_streaming_min_resident_mips,
        options.texture_streaming_max_resident_mips
    );

    return ideal_mip_count;
}

void render_texture_streamer::gather_texture_bounds(std::vector<render_view*>& views, std::vector<texture_bounds>& texture_bounds)
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();
    render_visibility_manager& visible_manager = m_renderer.get_visibility_manager();

    // Gather materials used by static meshes.
    std::vector<render_static_mesh*> static_meshes = scene_manager.get_static_meshes();
    for (render_view* view : views)
    {
        for (render_static_mesh* static_mesh : static_meshes)
        {
            if (visible_manager.is_object_visibile(view->get_visibility_view_id(), static_mesh->get_visibility_id()))
            {
                obb bounds = static_mesh->get_bounds();
                const asset_ptr<model>& mesh_model = static_mesh->get_model();

                // Go through sub meshes that are visible within this static mesh.
                for (size_t i = 0; i < mesh_model->meshes.size(); i++)
                {
                    const model::mesh_info& mesh_info = static_mesh->get_model()->meshes[i];
                    const asset_ptr<material>& mat = static_mesh->get_materials()[mesh_info.material_index];
                    if (!mat.is_loaded())
                    {
                        continue;
                    }

                    if (visible_manager.is_object_visibile(view->get_visibility_view_id(), static_mesh->get_submesh_visibility_id(i)))
                    {
                        for (const material::texture_info& tex : mat->textures)
                        {
                            if (!tex.texture.is_loaded())
                            {
                                continue;
                            }

                            if (tex.texture->streamed)
                            {
                                texture_bounds.emplace_back(
                                    tex.texture.get(), 
                                    view, 
                                    bounds, 
                                    mesh_info.min_texel_area,
                                    mesh_info.max_texel_area,
                                    mesh_info.min_world_area,
                                    mesh_info.max_world_area
                                );
                            }
                        }
                    }
                }
            }
        }
    }
}

void render_texture_streamer::calculate_in_view_mips()
{
    render_scene_manager& scene_manager = m_renderer.get_scene_manager();

    // Gather all views that can determine streaming state.
    std::vector<render_view*> views = scene_manager.get_views();
    std::erase_if(views, [](render_view* view) {
        return !view->should_render() ||
               !view->get_active() ||
               (view->get_flags() & render_view_flags::normal) != render_view_flags::normal;
    });
    
    // Gather a list of all materials.
    m_texture_bounds.clear();
    gather_texture_bounds(views, m_texture_bounds);

    // Calculate ideal mips for all textures.
    // TODO: Do this in parallel.
    for (texture_bounds& bounds : m_texture_bounds)
    {
        // TODO: Put an intrusive pointer in the texture asset to avoid this lookup.
        auto iter = m_streaming_textures.find(bounds.texture);
        if (iter == m_streaming_textures.end())
        {
            continue;
        }

        auto& info = *iter->second;
        size_t new_ideal_mips = calculate_ideal_mip_count(bounds);

        set_ideal_resident_mip_count(info, new_ideal_mips);
    }

    // If we have space, upgrade textures.

    // If we need more space for other textures to upgrade, downgrade unneeded textures.
}

void render_texture_streamer::async_update()
{
    calculate_in_view_mips();
}

}; // namespace ws
