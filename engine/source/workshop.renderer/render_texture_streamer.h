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
#include "workshop.render_interface/ri_staging_buffer.h"

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

    enum class texture_state
    {
        pending_upgrade,
        pending_downgrade,
        waiting_for_mips,
        waiting_for_downgrade,
        idle,

        COUNT
    };

    inline static const char* texture_state_strings[static_cast<int>(texture_state::COUNT)] = {
        "pending upgrade",
        "pending downgrade",
        "streaming",
        "waiting for downgrade",
        "idle"
    };

    struct texture_mip_request
    {
        size_t mip_index;
        async_io_request::ptr async_request;
        std::unique_ptr<ri_staging_buffer> staging_buffer;
    };

    struct texture_streaming_info
    {
        texture*            instance;
        texture_state       state;
        size_t              current_resident_mips;
        size_t              ideal_resident_mips;
        bool                can_decay = false;
        size_t              last_seen_frame = 0;
        std::atomic_size_t  locked_count { 0 };

        std::vector<texture_mip_request> mip_requests;
    };

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

    // Runs callback for every texture state the manager is currently handling. 
    // This is mostly here for debugging, its not fast and will block loading, so don't use it anywhere time critical.
    using visit_callback_t = std::function<void(const texture_streaming_info& state)>;
    void visit_textures(visit_callback_t callback);

    // Gets the number of bytes currently being used by streamed textures.
    // This is not perfectly accurate, tile pooling and fragmentation will effect this. But
    // it will be in the rough ballpark and is used for streaming heuristics.
    size_t get_memory_pressure();

    // Gets the ideal memory usage if all textures were at their ideal mip levels.
    size_t get_ideal_memory_usage();

    // Locking a texture forces the texture streamer to load it in fully resident, and it will
    // remain resident until unlock_texture is called.
    // 
    // Locked textures get priority for pool space over all others.
    //
    // Locks are reference counted (eg, two calls to lock_texture required two calls to unlock_texture to unlock it).
    void lock_texture(texture* tex);

    // Unlocking a texture forces the texture streamer to load it in fully resident.
    void unlock_texture(texture* tex);

    // Returns true if the given texture has all its streamed mips resident.
    // 
    // This doesn't mean that all mips in the reserved textures are streamed in, just all the possible
    // mips that can be streamed in are resident (ie. this takes cvar_texture_streaming_max_resident_mips clamping into account).
    bool is_texture_fully_resident(texture* tex);

private:
    friend class texture;

    struct texture_bounds
    {
        texture*        texture;
        render_view*    view;
        obb             bounds;

        float           min_texel_area;
        float           max_texel_area;
        float           avg_texel_area;
        float           min_world_area;
        float           max_world_area;
        float           avg_world_area;
        float           uv_density;
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

    // Calculates the number of resident mips in the given texture. Not the same as
    // get_current_resident_mip_count which just returns a cached value.
    size_t calculate_current_resident_mip_count(texture_streaming_info& streaming_info);

private:
    renderer& m_renderer;

    task_handle m_async_update_task;

    std::vector<texture_bounds> m_texture_bounds;

    std::array<std::vector<texture_streaming_info*>, (int)texture_state::COUNT> m_state_array;

    int64_t m_current_memory_pressure = 0;
    int64_t m_ideal_memory_pressure = 0;

    size_t m_total_staging_buffer_size = 0;

    std::shared_mutex m_access_mutex;
    std::unordered_map<texture*, std::shared_ptr<texture_streaming_info>> m_streaming_textures;

    bool m_pool_overcomitted = false;
};

}; // namespace ws
