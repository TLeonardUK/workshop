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

    // Gets the current number of mips the texture wants to be resident.
    size_t get_wanted_resident_mip_count(texture* tex);

    // Gets the current number of mips the texture nees for ideal rendering.
    size_t get_ideal_resident_mip_count(texture* tex);

    // Registers a texture that will be used by the streaming system.
    void register_texture(texture* tex);

    // Unregisters a texture that was previously registered with register_texture.
    void unregister_texture(texture* tex);

private:

    struct texture_streaming_info
    {
        texture* instance;
        size_t   wanted_resident_mips;
        size_t   ideal_resident_mips;
    };

    struct texture_bounds
    {
        texture*        texture;
        render_view*    view;
        obb             bounds;

        float           min_texel_area;
        float           max_texel_area;
        float           min_world_area;
        float           max_world_area;
    };

    // Called from a background thread to calculate which mips are needed and kick off any 
    // neccessary streaming work.
    void async_update();

    // Calculates which textures are in view.
    void calculate_in_view_mips();

    // Calculates the ideal mip level for the given texture bounds.
    size_t calculate_ideal_mip_count(const texture_bounds& bounds);

    // Sets the ideal resident mip count of a texture and updates any releated linkage.
    void set_ideal_resident_mip_count(texture_streaming_info& streaming_info, size_t new_ideal_mips);

    // Gathers the textures and the bounds of the objects they are applied to from the scene.
    void gather_texture_bounds(std::vector<render_view*>& views, std::vector<texture_bounds>& texture_bounds);

private:
    renderer& m_renderer;

    task_handle m_async_update_task;

    std::vector<texture_bounds> m_texture_bounds;

    std::vector<std::unique_ptr<texture_streaming_info>> m_pending_register_textures;
    std::unordered_map<texture*, std::unique_ptr<texture_streaming_info>> m_streaming_textures;
};

}; // namespace ws
