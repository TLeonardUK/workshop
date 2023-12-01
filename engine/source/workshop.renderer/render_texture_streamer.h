// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/async/task_scheduler.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/filesystem/async_io_manager.h"

#include <shared_mutex>
#include <vector>
#include <array>

namespace ws {

class renderer;
class texture;
class render_view;
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
    size_t get_current_resident_mip_count(texture* tex);

    // Gets the current number of mips the texture nees for ideal rendering.
    size_t get_ideal_resident_mip_count(texture* tex);

    // Registers a texture that will be used by the streaming system.
    void register_texture(texture* tex);

    // Unregisters a texture that was previously registered with register_texture.
    void unregister_texture(texture* tex);

private:
    friend class texture;

    enum class texture_state
    {
        pending_upgrade,
        pending_downgrade,
        waiting_for_mips,
        waiting_for_downgrade,
        idle,

        COUNT
    };

    struct texture_mip_request
    {
        size_t mip_index;
        async_io_request::ptr async_request;
    };

    struct texture_streaming_info
    {
        texture*        instance;
        texture_state   state;
        size_t          current_resident_mips;
        size_t          ideal_resident_mips;
        bool            can_decay = false;

        std::vector<texture_mip_request> mip_requests;
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

    // Changes the state of the texture.
    void set_texture_state(texture_streaming_info& streaming_info, texture_state new_state);

    // Removes a texture from the state array.
    void remove_from_state_array(texture_streaming_info& streaming_info);

    // Adds a texture from the state array.
    void add_to_state_array(texture_streaming_info& streaming_info);

    // Calculates the texture we should begin upgrading/downgrading.
    void calculate_textures_to_change(std::vector<texture_streaming_info*>& to_upgrade, std::vector<texture_streaming_info*>& to_downgrade);
   
    // Starts streaming in mip data for the texture to upgrade it.
    void start_upgrade(texture_streaming_info& streaming_info);

    // Starts dropping resident mips until texture is at its ideal mips.
    void start_downgrade(texture_streaming_info& streaming_info);

    // Checks textures that are currently streaming mips, if they have completed
    // it makes the mips resident and returns the texture to the idle state.
    void make_completed_mips_resident();

    // Makes all downgraded textures non-resident.
    void make_downgrades_non_resident();

private:
    renderer& m_renderer;

    task_handle m_async_update_task;

    std::vector<texture_bounds> m_texture_bounds;

    std::array<std::vector<texture_streaming_info*>, (int)texture_state::COUNT> m_state_array;

    int64_t m_current_memory_pressure = 0;

    std::shared_mutex m_access_mutex;
    std::unordered_map<texture*, std::shared_ptr<texture_streaming_info>> m_streaming_textures;

    bool m_pool_overcomitted = false;
};

}; // namespace ws
