// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_texture_streamer.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/systems/render_system_debug.h"
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/objects/render_view.h"

#include "workshop.core/perf/profile.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/memory/memory_tracker.h"

#include "workshop.core/math/plane.h"

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

size_t render_texture_streamer::get_current_resident_mip_count(texture* tex)
{
    std::shared_lock lock(m_access_mutex);

    if (auto iter = m_streaming_textures.find(tex); iter != m_streaming_textures.end())
    {
        return iter->second->current_resident_mips;
    }

    const render_options& options = m_renderer.get_options();
    return options.texture_streaming_min_resident_mips;
}

size_t render_texture_streamer::get_ideal_resident_mip_count(texture* tex)
{
    std::shared_lock lock(m_access_mutex);

    if (auto iter = m_streaming_textures.find(tex); iter != m_streaming_textures.end())
    {
        return iter->second->ideal_resident_mips;
    }

    const render_options& options = m_renderer.get_options();
    return options.texture_streaming_min_resident_mips;
}

void render_texture_streamer::register_texture(texture* tex)
{
    std::unique_lock lock(m_access_mutex);

    const render_options& options = m_renderer.get_options();

    std::shared_ptr<texture_streaming_info> info = std::make_shared<texture_streaming_info>();
    info->instance = tex;
    info->current_resident_mips = tex->ri_instance->get_resident_mips();
    info->ideal_resident_mips = info->current_resident_mips;
    info->state = texture_state::idle;
    tex->streaming_info = info;

    db_assert(tex->streamed);
    db_assert(tex->ri_instance->is_partially_resident());

    m_current_memory_pressure += tex->ri_instance->get_memory_usage_with_residency(info->current_resident_mips);

    add_to_state_array(*tex->streaming_info);

    m_streaming_textures[tex] = std::move(info);
}

void render_texture_streamer::unregister_texture(texture* tex)
{
    std::unique_lock lock(m_access_mutex);

    // Ensure all mip staging is complete before we unregister the texture.
    for (texture_mip_request& request : tex->streaming_info->mip_requests)
    {
        if (request.staging_buffer)
        {
            request.staging_buffer->wait();
        }
    }

    m_streaming_textures.erase(tex);

    remove_from_state_array(*tex->streaming_info);
}

void render_texture_streamer::begin_frame()
{
    const render_options& options = m_renderer.get_options();
    if (!options.texture_streaming_enabled)
    {
        return;
    }

    m_async_update_task = async("texture streamer update", task_queue::standard, [this]() {
        profile_marker(profile_colors::render, "texture streaming update");
        async_update();
    });
}

void render_texture_streamer::end_frame()
{
    profile_marker(profile_colors::render, "texture streaming integration");

    const render_options& options = m_renderer.get_options();
    if (!options.texture_streaming_enabled)
    {
        return;
    }

    // Wait for current update task to complete.
    if (m_async_update_task.is_valid())
    {
        m_async_update_task.wait(true);
        m_async_update_task.reset();
    }

    make_completed_mips_resident();
    make_downgrades_non_resident();
}

void render_texture_streamer::visit_textures(visit_callback_t callback)
{
    std::unique_lock lock(m_access_mutex);

    for (auto& info : m_streaming_textures)
    {
        callback(*info.second);
    }
}

size_t render_texture_streamer::get_memory_pressure()
{
    return m_current_memory_pressure;
}

void render_texture_streamer::make_completed_mips_resident()
{
    const render_options& options = m_renderer.get_options();

    // Apply any textures that have finished streaming.
    std::vector<texture_streaming_info*>& waiting_list = m_state_array[(int)texture_state::waiting_for_mips];

    std::vector<texture_streaming_info*> failed_upgrades;
    std::vector<texture_streaming_info*> success_upgrades;

    double start = get_seconds();
    bool time_elapsed = false;
    size_t mips_made_resident = 0;

    size_t original_waiting_list_size = waiting_list.size();

    for (texture_streaming_info* info : waiting_list)
    {
        bool any_mips_failed = false;

        // Check each mip request we are pending.
        for (auto mip_iter = info->mip_requests.begin(); mip_iter != info->mip_requests.end(); /*empty*/)
        {
            if (mip_iter->async_request->is_complete())
            {
                if (mip_iter->async_request->has_failed())
                {
                    any_mips_failed = true;
                }
                else if (!mip_iter->staging_buffer)
                {
                    if (m_total_staging_buffer_size < options.texture_streaming_max_staged_memory)
                    {
                        // Asyncronously copy data to an upload buffer.
                        ri_staging_buffer::create_params params;
                        params.destination = info->instance->ri_instance.get();
                        params.mip_index = mip_iter->mip_index;
                        params.array_index = 0;

                        mip_iter->staging_buffer = m_renderer.get_render_interface().create_staging_buffer(params, mip_iter->async_request->data());
                        m_total_staging_buffer_size += mip_iter->async_request->data().size();
                    }
                    else
                    {
                        mip_iter++;
                    }
                }
                else if (mip_iter->staging_buffer->is_staged())
                {
                    info->instance->ri_instance->begin_mip_residency_change();
                    info->instance->ri_instance->make_mip_resident(mip_iter->mip_index, *mip_iter->staging_buffer);
                    info->instance->ri_instance->end_mip_residency_change();

                    m_total_staging_buffer_size -= mip_iter->async_request->data().size();
                    mip_iter->staging_buffer = nullptr;

                    mips_made_resident++;
                    mip_iter = info->mip_requests.erase(mip_iter);
                }
                else
                {
                    mip_iter++;
                }
            }
            else
            {
                mip_iter++;
            }

            // See if time has elapsed and stop fulfilling other mip requests.
            double elapsed = (get_seconds() - start) * 1000.0f;
            if (elapsed >= options.texture_streaming_time_limit_ms)
            {
                time_elapsed = true;
                break;
            }
        }

        // Move out of waiting for mips when complete.
        if (any_mips_failed)
        {
            failed_upgrades.push_back(info);
        }
        else if (info->mip_requests.empty())
        {
            success_upgrades.push_back(info);
        }

        // Stop processing if time has elapsed.
        if (time_elapsed)
        {
            break;
        }
    }

    // Switch states of completed (or failed) textures.
    for (texture_streaming_info* info : failed_upgrades)
    {
        info->mip_requests.clear();

        set_texture_state(*info, texture_state::idle);
    }
    for (texture_streaming_info* info : success_upgrades)
    {
        info->mip_requests.clear();

        m_current_memory_pressure -= info->instance->ri_instance->get_memory_usage_with_residency(info->current_resident_mips);
        info->current_resident_mips = info->ideal_resident_mips;
        m_current_memory_pressure += info->instance->ri_instance->get_memory_usage_with_residency(info->current_resident_mips);

        set_texture_state(*info, texture_state::idle);
    }
    
    double elapsed = (get_seconds() - start) * 1000.0f;
    if (!success_upgrades.empty() && elapsed > 2.0f)
    {
        db_log(core, "mips:%zi time:%.2f completed:%zi remaining:%zi memory:%.2f mb", mips_made_resident, elapsed, original_waiting_list_size, waiting_list.size(), m_current_memory_pressure / (1024.0f * 1024.0f));
    }
}

void render_texture_streamer::make_downgrades_non_resident()
{
    std::vector<texture_streaming_info*> to_downgrade = m_state_array[(int)texture_state::waiting_for_downgrade];

    for (texture_streaming_info* info : to_downgrade)
    {
        info->instance->ri_instance->begin_mip_residency_change();

        for (size_t i = info->ideal_resident_mips; i <= info->current_resident_mips; i++)
        {
            size_t mip_index = info->instance->ri_instance->get_mip_levels() - i;

            //db_log(renderer, "Made mip %zi non-resident for %s", mip_index, info->instance->name.c_str());

            info->instance->ri_instance->make_mip_non_resident(mip_index);
        }

        info->instance->ri_instance->end_mip_residency_change();

        m_current_memory_pressure -= info->instance->ri_instance->get_memory_usage_with_residency(info->current_resident_mips);
        info->current_resident_mips = info->ideal_resident_mips;
        m_current_memory_pressure += info->instance->ri_instance->get_memory_usage_with_residency(info->current_resident_mips);

        set_texture_state(*info, texture_state::idle);
    }
}

void render_texture_streamer::set_ideal_resident_mip_count(texture_streaming_info& streaming_info, size_t new_ideal_mips)
{
    if (streaming_info.ideal_resident_mips == new_ideal_mips)
    {
        return;
    }

    //db_log(renderer, "[Streaming] Ideal mip changed to %zi (current mips %zi) for %s", new_ideal_mips, streaming_info.current_resident_mips, streaming_info.instance->name.c_str());
    streaming_info.ideal_resident_mips = new_ideal_mips;

    // Switch state based on the delta between current and ideal mips.
    if (streaming_info.ideal_resident_mips > streaming_info.current_resident_mips)
    {
        set_texture_state(streaming_info, texture_state::pending_upgrade);
    }
    else if (streaming_info.ideal_resident_mips < streaming_info.current_resident_mips)
    {
        set_texture_state(streaming_info, texture_state::pending_downgrade);
    }
    else
    {
        set_texture_state(streaming_info, texture_state::idle);
    }
}

void render_texture_streamer::set_texture_state(texture_streaming_info& streaming_info, texture_state new_state)
{
    if (streaming_info.state == new_state)
    {
        return;
    }

    //db_log(renderer, "[Streaming] Changed state to %zi for %s", (size_t)new_state, streaming_info.instance->name.c_str());

    remove_from_state_array(streaming_info);
    streaming_info.state = new_state;
    add_to_state_array(streaming_info);
}

void render_texture_streamer::remove_from_state_array(texture_streaming_info& streaming_info)
{
    auto& state_array = m_state_array[(int)streaming_info.state];
    auto iter = std::find(state_array.begin(), state_array.end(), &streaming_info);
    if (iter != state_array.end())
    {
        state_array.erase(iter);
    }
}

void render_texture_streamer::add_to_state_array(texture_streaming_info& streaming_info)
{
    auto& state_array = m_state_array[(int)streaming_info.state];
    state_array.push_back(&streaming_info);
}

size_t render_texture_streamer::calculate_ideal_mip_count(const texture_bounds& tex_bounds)
{
    const render_options& options = m_renderer.get_options();

#if 1

    // More janky and more expensive, but at least this produces the results that are desired ...

    matrix4 view_matrix = tex_bounds.view->get_view_matrix() * tex_bounds.view->get_projection_matrix();

    vector3 view_normal = tex_bounds.view->get_transform().transform_direction(vector3::forward).normalize();
    plane view_plane(view_normal, tex_bounds.view->get_local_location());

    // Calculate the bounds of the OOB in screen space.
    vector3 screen_space_corners[8];
    tex_bounds.bounds.get_corners(screen_space_corners);
    for (size_t i = 0; i < 8; i++)
    {
        vector3& corner = screen_space_corners[i];

        /// huuuum, calculate world space area and convert that view space area?
        // Right now this gets very squirrely when its partially offscreen.

        // Project corners onto view plane.
        corner = view_plane.project(corner);

        // Convert to clip space coordinates.
        corner = corner * view_matrix;

        // Perspective divide.
        corner = vector3(
            corner.x / corner.z,
            corner.y / corner.z,
            1.0f
        );

        /*
        // Convert to NDC
#if 1
        corner = vector3(
            (corner.x + 1.0f) * 0.5f,
            (-corner.y + 1.0f) * 0.5f,
            0.0f
        );  
#else
        corner = vector3(
            std::clamp((corner.x + 1.0f) * 0.5f, 0.0f, 1.0f),
            std::clamp((-corner.y + 1.0f) * 0.5f, 0.0f, 1.0f),
            0.0f
        );
#endif
        */

        // Convert to viewport position
        corner *= vector3(
            (float)tex_bounds.view->get_viewport().width, 
            (float)tex_bounds.view->get_viewport().height, 
            1.0f
        );
    }

    aabb screen_space_bounds(screen_space_corners, 8);
    vector3 screen_space_extents = screen_space_bounds.get_extents() * 2.0f;
    float screen_space_area = std::pow(std::max(screen_space_extents.x, screen_space_extents.y), 2.0f);

    // Calculate the texel density of the mesh.
    float texel_density = (tex_bounds.max_texel_area) / (tex_bounds.min_world_area);

    // Calculate the ideal mip count.
    size_t mip_count = tex_bounds.texture->ri_instance->get_mip_levels();
    float ideal_mip_float = 0.5f * log2(screen_space_area / texel_density);
    size_t ideal_mip_count = (size_t)std::clamp((int)std::truncf(ideal_mip_float) + options.texture_streaming_mip_bias, 0, (int)mip_count);

    //db_log(core, "screen_space_area=%.2f texel_density=%.2f mip=%zi", screen_space_area, texel_density, ideal_mip_count);

#else
    // Calculations here are based roughly on this:
    // http://web.cse.ohio-state.edu/~crawfis.3/cse781/Readings/MipMapLevels-Blog.html

    // Find closest point on bounds to camera.
    vector3 view_location = tex_bounds.view->get_local_location();
    vector3 closet_point = tex_bounds.bounds.get_closest_point(view_location);

    aabb bounds = tex_bounds.bounds.get_aligned_bounds();

#if 0
    m_renderer.get_command_queue().draw_sphere(sphere(closet_point, 50.0f), color::red);
    m_renderer.get_command_queue().draw_obb(tex_bounds.bounds, color::green);
#endif

    // Find the screen space area.
    float view_width = (float)tex_bounds.view->get_viewport().width;
    float distance = ((closet_point - view_location).length() + 0.001f); // small epsilon to avoid issues when inside meshes.
    float fov = tex_bounds.view->get_fov();
    float a = distance * std::tanf(math::radians(fov));
    float q = std::powf(view_width / a, 2);

    // Determine how many mips to drop based on the texel density of the mesh.
    float world_scale = tex_bounds.bounds.transform.extract_scale().max_component();
    float texel_factor = (tex_bounds.min_texel_area * tex_bounds.texture->width) / (tex_bounds.min_world_area * world_scale);
    float mips_to_drop = 0.5f * log2(texel_factor / q);

    // Calculate the ideal mip count.
    size_t mip_count = tex_bounds.texture->ri_instance->get_mip_levels();

    // We always add 1 to the ideal mip count for trilinear filtering.
    size_t ideal_mip_count = (size_t)std::clamp(((int)mip_count - (int)std::truncf(mips_to_drop)) + 1, 0, (int)mip_count); 

#if 1
    db_log(core, "a=%.2f q=%.2f factor=%.2f ideal=%zi", a, q, mips_to_drop, ideal_mip_count);
#endif
#endif

    // Clamp to minimum and maximum mip bounds.
    ideal_mip_count = std::clamp(
        ideal_mip_count,
        std::min(mip_count, options.texture_streaming_min_resident_mips),
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
                //obb bounds = static_mesh->get_bounds();
                const asset_ptr<model>& mesh_model = static_mesh->get_model();
                if (!mesh_model.is_loaded())
                {
                    continue;
                }

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
                        obb mesh_bounds = obb(mesh_info.bounds, static_mesh->get_transform());

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
                                    mesh_bounds,
                                    mesh_info.min_texel_area,
                                    mesh_info.max_texel_area,
                                    mesh_info.avg_texel_area,
                                    mesh_info.min_world_area,
                                    mesh_info.max_world_area,
                                    mesh_info.avg_world_area
                                );
                            }
                        }
                    }
                }
            }
        }
    }
}

void render_texture_streamer::start_upgrade(texture_streaming_info& streaming_info)
{
    //db_log(renderer, "Started streaming texture mips for: %s", streaming_info.instance->name.c_str());

    // Queue up requests for each mip we are missing.
    for (size_t i = 0; i < streaming_info.ideal_resident_mips; i++)
    {
        size_t mip_index = streaming_info.instance->ri_instance->get_mip_levels() - (i + 1);

        if (!streaming_info.instance->ri_instance->is_mip_resident(mip_index))
        {
            size_t mip_data_offset;
            size_t mip_data_size;

            streaming_info.instance->ri_instance->get_mip_source_data_range(mip_index, mip_data_offset, mip_data_size);

            //db_log(renderer, "mip[%zi] offset=%zi size=%zi path=%s", mip_index, mip_data_offset, mip_data_size, streaming_info.instance->name.c_str());

            texture_mip_request& request = streaming_info.mip_requests.emplace_back();
            request.mip_index = mip_index;
            request.async_request = async_io_manager::get().request(
                streaming_info.instance->async_data_path.c_str(),
                streaming_info.instance->async_data_offset + mip_data_offset,
                mip_data_size,
                async_io_request_options::none
            );
        }
    }

    db_assert(!streaming_info.mip_requests.empty());

    set_texture_state(streaming_info, texture_state::waiting_for_mips);
}

void render_texture_streamer::start_downgrade(texture_streaming_info& streaming_info)
{
    set_texture_state(streaming_info, texture_state::waiting_for_downgrade);
}

void render_texture_streamer::calculate_textures_to_change(std::vector<texture_streaming_info*>& to_upgrade, std::vector<texture_streaming_info*>& to_downgrade)
{
    const render_options& options = m_renderer.get_options();

    int64_t bytes_available = (int64_t)options.texture_streaming_pool_size - (int64_t)m_current_memory_pressure;

    // Reduce the available bytes by the pressure of pending upgrades.
    for (texture_streaming_info* texture : m_state_array[(int)texture_state::waiting_for_mips])
    {
        size_t current_size = texture->instance->ri_instance->get_memory_usage_with_residency(texture->current_resident_mips);
        size_t ideal_size = texture->instance->ri_instance->get_memory_usage_with_residency(texture->ideal_resident_mips);
        size_t memory_delta = ideal_size - current_size;

        bytes_available -= memory_delta;
    }

    // Try to upgrade as many textures as fit into the remaining pool space.
    for (texture_streaming_info* texture : m_state_array[(int)texture_state::pending_upgrade])
    {
        size_t current_size = texture->instance->ri_instance->get_memory_usage_with_residency(texture->current_resident_mips);
        size_t ideal_size = texture->instance->ri_instance->get_memory_usage_with_residency(texture->ideal_resident_mips);
        size_t memory_delta = ideal_size - current_size;

        if ((int64_t)memory_delta < bytes_available)
        {
            to_upgrade.push_back(texture);
        }

        bytes_available -= memory_delta;
    }

    // If we need more space for other textures to upgrade, downgrade unneeded textures.
    if (bytes_available < 0 || options.texture_streaming_force_unstream)
    {
        for (texture_streaming_info* texture : m_state_array[(int)texture_state::pending_downgrade])
        {
            size_t current_size = texture->instance->ri_instance->get_memory_usage_with_residency(texture->current_resident_mips);
            size_t ideal_size = texture->instance->ri_instance->get_memory_usage_with_residency(texture->ideal_resident_mips);
            size_t memory_delta = current_size - ideal_size;

            to_downgrade.push_back(texture);
            bytes_available += memory_delta;

            if (bytes_available >= 0 && !options.texture_streaming_force_unstream)
            {
                break;
            }
        }
    }

    if (bytes_available < 0)
    {
        if (!m_pool_overcomitted)
        {
            db_warning(renderer, "Texture streamer is overcommitted, ideal mips for all active textures are larger than the pool size. Consider increasing the pool size.");
            m_pool_overcomitted = true;
        }
    }
    else
    {
        m_pool_overcomitted = false;
    }
}

void render_texture_streamer::calculate_in_view_mips()
{
    std::shared_lock lock(m_access_mutex);

    const render_options& options = m_renderer.get_options();
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
    std::unordered_map<texture*, size_t> highest_texture_mips;

    for (texture_bounds& bounds : m_texture_bounds)
    {
        // Don't update any textures that are currently awaiting mip downloads.
        if (bounds.texture->streaming_info->state == texture_state::waiting_for_mips)
        {
            continue;
        }

        size_t new_ideal_mips = calculate_ideal_mip_count(bounds);
        highest_texture_mips[bounds.texture] = std::max(highest_texture_mips[bounds.texture], new_ideal_mips);
    }

    for (auto& [texture, highest_mip] : highest_texture_mips)
    {
        set_ideal_resident_mip_count(*texture->streaming_info.get(), highest_mip);

        // Don't allow this texture to decay as its been seen this frame.
        texture->streaming_info->can_decay = false;
    }

    // If we didn't see a texture this frame, decay it.
    // (This won't actually stream the texture out unless the texture pool is under heavy pressure).
    for (auto& [ texture, info ] : m_streaming_textures)
    {
        // Don't decay textures which are currently in the process of streaming.
        if (info->state == texture_state::waiting_for_mips)
        {
            continue;
        }

        // Check the texture wasn't seen this frame.
        if (info->can_decay)
        {
            size_t mip_count = texture->ri_instance->get_mip_levels();
            set_ideal_resident_mip_count(*info, std::min(mip_count, options.texture_streaming_min_resident_mips));
        }

        info->can_decay = true;
    }

    // Calculate the changes we should make this frame.
    std::vector<texture_streaming_info*> to_upgrade;
    std::vector<texture_streaming_info*> to_downgrade;
    calculate_textures_to_change(to_upgrade, to_downgrade);

    // Kick off streaming in mip data.
    for (texture_streaming_info* info : to_upgrade)
    {
        start_upgrade(*info);
    }

    // Kick off downgrades
    for (texture_streaming_info* info : to_downgrade)
    {
        start_downgrade(*info);
    }
}

void render_texture_streamer::async_update()
{
    calculate_in_view_mips();
}

}; // namespace ws
