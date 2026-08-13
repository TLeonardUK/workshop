// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_tile_manager.h"

namespace ws {

vulkan_ri_tile_manager::vulkan_ri_tile_manager(vulkan_render_interface& renderer)
{
}

vulkan_ri_tile_manager::~vulkan_ri_tile_manager() = default;

result<void> vulkan_ri_tile_manager::create_resources()
{
    return true;
}

void vulkan_ri_tile_manager::begin_frame()
{
}

vulkan_ri_tile_manager::allocation vulkan_ri_tile_manager::allocate_tiles(size_t tile_count)
{
    return allocation{};
}

void vulkan_ri_tile_manager::free_tiles(allocation alloc)
{
}

void vulkan_ri_tile_manager::queue_map(vulkan_ri_texture& texture, allocation alloc, size_t mip_index)
{
}

void vulkan_ri_tile_manager::queue_unmap(vulkan_ri_texture& texture, size_t mip_index)
{
}

}; // namespace ws
