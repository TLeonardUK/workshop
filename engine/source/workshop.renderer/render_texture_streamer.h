// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"

#include "workshop.renderer/assets/texture/texture.h"

namespace ws {

class renderer;
class texture;
class input_interface;

// ================================================================================================
//  Handles streaming in/out of textures dynamically at runtime.
// ================================================================================================
class render_texture_streamer
{
public:
    render_texture_streamer(renderer& renderer);
    ~render_texture_streamer();

    // Registers all the steps required to initialize the system.
    void register_init(init_list& list);

    // Called when a new render frame is starting. The world state will have finished being updated
    // by this point and a new texture streaming task can be kicked off.
    void begin_frame();

    // Called before starting to update the world state for the next frame. Texture streaming task should
    // be joined at this point and any changes processed.
    void end_frame();

/*
    // Register an arbitrary object made up of a set of textures and a world space bounding volume.
    // This will be used to determining when textures need to load in.
    object_id register_object(texture& texture, obb bounds);

    // Updates the textures a given object references.
    void update_object_textures(texture& texture);

    // Updates a given objects world space bounds.
    void update_object_bounds(obb bounds);

    // Unregister a previously registered object.
    void unregister_object(object_id id);

    // Registers a view used to determine which objects are in view and need their 
    // textures streamed in.
    view_id register_view(const frustum& frustum);

    // Updates the frustum of a previously registered view.
    void update_view_frustum(const frustum& frustum);

    // Unregisters a previously registered view.
    void unregister_view(view_id id);
*/

    // Gets the current number of mips the texture wants to be resident.
    size_t get_resident_mip_count(texture* tex);

    // Registers a texture that will be used by the streaming system.
    void register_texture(texture* tex);

    // Unregisters a texture that was previously registered with register_texture.
    void unregister_texture(texture* tex);

private:

    // Called from a background thread to calculate which mips are needed and kick off any 
    // neccessary streaming work.
    void async_update();

    // Calculates which textures are in view.
    void calculate_in_view_mips();

private:
    renderer& m_renderer;

    task_handle m_async_update_task;

    struct texture_streaming_info
    {
        texture* instance;
        size_t resident_mips;
    };

    std::vector<std::unique_ptr<texture_streaming_info>> m_pending_register_textures;
    std::unordered_map<texture*, std::unique_ptr<texture_streaming_info>> m_streaming_textures;
};

}; // namespace ws
