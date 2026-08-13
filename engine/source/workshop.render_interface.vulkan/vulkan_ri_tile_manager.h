// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.core/utils/result.h"

#include <vector>

namespace ws {

class vulkan_render_interface;
class vulkan_ri_texture;

// ================================================================================================
//  This class manages the creation and updating of tiles for reserved resources.
// ================================================================================================
class vulkan_ri_tile_manager
{
public:
    struct allocation
    {
        bool is_valid = false;
    };

public:
    vulkan_ri_tile_manager(vulkan_render_interface& renderer);
    ~vulkan_ri_tile_manager();

    result<void> create_resources();

    void begin_frame();

    allocation allocate_tiles(size_t tile_count);
    void free_tiles(allocation alloc);

    void queue_map(vulkan_ri_texture& texture, allocation alloc, size_t mip_index);
    void queue_unmap(vulkan_ri_texture& texture, size_t mip_index);

    static constexpr size_t k_tile_byte_size = 64 * 1024;

};

}; // namespace ws
