// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_tile_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_texture.h"
#include "workshop.core/math/math.h"

namespace ws {

vulkan_ri_tile_manager::vulkan_ri_tile_manager(vulkan_render_interface& renderer)
    : m_renderer(renderer)
{
}

vulkan_ri_tile_manager::~vulkan_ri_tile_manager()
{
    if (m_bind_sparse_fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(m_renderer.get_device(), m_bind_sparse_fence, nullptr);
    }

    for (std::unique_ptr<heap_state>& heap : m_heaps)
    {
        vkFreeMemory(m_renderer.get_device(), heap->memory, nullptr);
    }
    m_heaps.clear();
}

result<void> vulkan_ri_tile_manager::create_resources()
{
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkResult vk_result = vkCreateFence(m_renderer.get_device(), &fence_create_info, nullptr, &m_bind_sparse_fence);
    if (!m_renderer.check_result(vk_result, "vkCreateFence"))
    {
        return standard_errors::failed;
    }

    return true;
}

void vulkan_ri_tile_manager::allocate_new_heap(size_t minimum_size_in_tiles)
{
    memory_scope mem_scope(memory_type::rendering__vram__tile_heap, memory_scope::k_ignore_asset);

    std::unique_ptr<heap_state> state = std::make_unique<heap_state>();
    state->size_in_tiles = math::round_up_multiple(minimum_size_in_tiles, k_heap_granularity_in_tiles);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = state->size_in_tiles * k_tile_byte_size;

    result<uint32_t> memory_type_index = m_renderer.find_memory_type(~0u, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    db_assert(memory_type_index);
    allocate_info.memoryTypeIndex = memory_type_index.get_result();

    VkResult vk_result = vkAllocateMemory(m_renderer.get_device(), &allocate_info, nullptr, &state->memory);
    m_renderer.assert_result(vk_result, "vkAllocateMemory");

    state->heap = std::make_unique<memory_heap>(state->size_in_tiles);

    state->slack_memory_allocation_info = mem_scope.record_alloc(state->heap->get_remaining() * k_tile_byte_size);

    m_heaps.push_back(std::move(state));
}

vulkan_ri_tile_manager::allocation vulkan_ri_tile_manager::allocate_tiles(size_t tile_count)
{
    std::scoped_lock lock(m_mutex);

    memory_scope mem_scope(memory_type::rendering__vram__tile_heap, memory_scope::k_ignore_asset);

    allocation result_allocation;
    result_allocation.is_valid = true;

    // Unlike dx12, a single allocation is never split across multiple heaps - this keeps
    // the resulting bind region a single contiguous VkSparseImageMemoryBind rather than
    // requiring 2d-region-splitting math across heap boundaries.
    for (std::unique_ptr<heap_state>& heap : m_heaps)
    {
        heap_tile_allocation heap_allocation;
        heap_allocation.heap = heap.get();
        heap_allocation.tile_count = tile_count;

        if (heap->heap->alloc(tile_count, 1, heap_allocation.tile_offset))
        {
            result_allocation.heap_allocations.push_back(heap_allocation);

            heap->slack_memory_allocation_info = mem_scope.record_alloc(heap->heap->get_remaining() * k_tile_byte_size);

            return result_allocation;
        }
    }

    db_log(core, "Allocating new tile heap.");
    allocate_new_heap(tile_count);

    heap_state* heap = m_heaps.back().get();

    heap_tile_allocation heap_allocation;
    heap_allocation.heap = heap;
    heap_allocation.tile_count = tile_count;

    bool allocated = heap->heap->alloc(tile_count, 1, heap_allocation.tile_offset);
    db_assert(allocated);

    result_allocation.heap_allocations.push_back(heap_allocation);
    heap->slack_memory_allocation_info = mem_scope.record_alloc(heap->heap->get_remaining() * k_tile_byte_size);

    return result_allocation;
}

void vulkan_ri_tile_manager::free_tiles(allocation alloc)
{
    if (!alloc.is_valid)
    {
        return;
    }

    std::scoped_lock lock(m_mutex);

    operation& op = m_operations.emplace_back();
    op.type = operation_type::free_tiles;
    op.alloc = alloc;
    // Free only after pipeline depth has elapsed so we can be assured the tiles are no longer in use.
    op.frame_index = m_frame_index + m_renderer.get_pipeline_depth();
}

void vulkan_ri_tile_manager::queue_map(vulkan_ri_texture& texture, allocation alloc, size_t mip_index)
{
    std::scoped_lock lock(m_mutex);

    auto iter = std::find_if(m_operations.begin(), m_operations.end(), [&texture, &mip_index](const operation& op) {
        return (op.mip_index == mip_index && op.texture == &texture) && (op.type == operation_type::map || op.type == operation_type::unmap);
    });
    if (iter != m_operations.end())
    {
        m_operations.erase(iter);
    }

    const vulkan_ri_texture::mip_residency& residency = texture.get_mip_residency(mip_index);

    operation& op = m_operations.emplace_back();
    op.type = operation_type::map;
    op.alloc = alloc;
    op.texture = &texture;
    op.image = texture.get_image();
    op.mip_index = mip_index;
    op.is_packed = residency.is_packed;
    op.aspect = texture.get_aspect_mask();
    op.pixel_extent = residency.pixel_extent;
    op.mip_tail_offset = texture.get_mip_tail_offset();
    op.mip_tail_size = texture.get_mip_tail_size();
    // We can map immediately - it'll be included in this frame's begin_frame() bind-sparse batch.
    op.frame_index = m_frame_index;
}

void vulkan_ri_tile_manager::queue_unmap(vulkan_ri_texture& texture, size_t mip_index)
{
    std::scoped_lock lock(m_mutex);

    auto iter = std::find_if(m_operations.begin(), m_operations.end(), [&texture, &mip_index](const operation& op) {
        return (op.mip_index == mip_index && op.texture == &texture) && (op.type == operation_type::map || op.type == operation_type::unmap);
    });
    if (iter != m_operations.end())
    {
        m_operations.erase(iter);
    }

    const vulkan_ri_texture::mip_residency& residency = texture.get_mip_residency(mip_index);

    operation& op = m_operations.emplace_back();
    op.type = operation_type::unmap;
    op.texture = &texture;
    op.image = texture.get_image();
    op.mip_index = mip_index;
    op.is_packed = residency.is_packed;
    op.aspect = texture.get_aspect_mask();
    op.pixel_extent = residency.pixel_extent;
    op.mip_tail_offset = texture.get_mip_tail_offset();
    op.mip_tail_size = texture.get_mip_tail_size();
    // Free only after pipeline depth has elapsed so we can be assured the tiles are no longer in use.
    op.frame_index = m_frame_index + m_renderer.get_pipeline_depth();
}

void vulkan_ri_tile_manager::submit_bind_sparse(const std::vector<operation*>& ops)
{
    if (ops.empty())
    {
        return;
    }

    // One VkSparseMemoryBind/VkSparseImageMemoryBind per operation, plus a parallel vector of
    // owning images so the per-op VkSparse*BindInfo structs can be built once bind storage
    // (which must outlive the vkQueueBindSparse call) has stopped growing and reallocating.
    std::vector<VkImage> opaque_images;
    std::vector<VkSparseMemoryBind> opaque_binds;
    std::vector<VkImage> image_bind_images;
    std::vector<VkSparseImageMemoryBind> image_binds;

    for (operation* op : ops)
    {
        if (op->is_packed)
        {
            VkSparseMemoryBind bind = {};
            bind.resourceOffset = op->mip_tail_offset;
            bind.size = op->mip_tail_size;

            if (op->type == operation_type::map && !op->alloc.heap_allocations.empty())
            {
                const heap_tile_allocation& heap_alloc = op->alloc.heap_allocations[0];
                bind.memory = heap_alloc.heap->memory;
                bind.memoryOffset = heap_alloc.tile_offset * k_tile_byte_size;
            }

            opaque_images.push_back(op->image);
            opaque_binds.push_back(bind);
        }
        else
        {
            VkSparseImageMemoryBind bind = {};
            bind.subresource.aspectMask = op->aspect;
            bind.subresource.mipLevel = static_cast<uint32_t>(op->mip_index);
            bind.subresource.arrayLayer = 0;
            bind.offset = { 0, 0, 0 };
            bind.extent = op->pixel_extent;

            if (op->type == operation_type::map && !op->alloc.heap_allocations.empty())
            {
                const heap_tile_allocation& heap_alloc = op->alloc.heap_allocations[0];
                bind.memory = heap_alloc.heap->memory;
                bind.memoryOffset = heap_alloc.tile_offset * k_tile_byte_size;
            }

            image_bind_images.push_back(op->image);
            image_binds.push_back(bind);
        }
    }

    std::vector<VkSparseImageOpaqueMemoryBindInfo> opaque_infos;
    for (size_t i = 0; i < opaque_binds.size(); i++)
    {
        VkSparseImageOpaqueMemoryBindInfo info = {};
        info.image = opaque_images[i];
        info.bindCount = 1;
        info.pBinds = &opaque_binds[i];
        opaque_infos.push_back(info);
    }

    std::vector<VkSparseImageMemoryBindInfo> per_image_infos;
    for (size_t i = 0; i < image_binds.size(); i++)
    {
        VkSparseImageMemoryBindInfo info = {};
        info.image = image_bind_images[i];
        info.bindCount = 1;
        info.pBinds = &image_binds[i];
        per_image_infos.push_back(info);
    }

    VkBindSparseInfo bind_sparse_info = {};
    bind_sparse_info.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
    bind_sparse_info.imageOpaqueBindCount = static_cast<uint32_t>(opaque_infos.size());
    bind_sparse_info.pImageOpaqueBinds = opaque_infos.data();
    bind_sparse_info.imageBindCount = static_cast<uint32_t>(per_image_infos.size());
    bind_sparse_info.pImageBinds = per_image_infos.data();

    vulkan_ri_command_queue& queue = static_cast<vulkan_ri_command_queue&>(m_renderer.get_graphics_queue());

    vkResetFences(m_renderer.get_device(), 1, &m_bind_sparse_fence);
    m_renderer.assert_result(vkQueueBindSparse(queue.get_queue(), 1, &bind_sparse_info, m_bind_sparse_fence), "vkQueueBindSparse");
    vkWaitForFences(m_renderer.get_device(), 1, &m_bind_sparse_fence, VK_TRUE, UINT64_MAX);
}

void vulkan_ri_tile_manager::perform_operations(size_t frame_index)
{
    std::vector<operation*> due_binds;

    for (auto iter = m_operations.begin(); iter != m_operations.end(); /* empty */)
    {
        operation& op = *iter;
        if (op.frame_index <= frame_index)
        {
            if (op.type == operation_type::free_tiles)
            {
                for (heap_tile_allocation& heap_alloc : op.alloc.heap_allocations)
                {
                    heap_alloc.heap->heap->free(heap_alloc.tile_offset);

                    memory_scope mem_scope(memory_type::rendering__vram__tile_heap, memory_scope::k_ignore_asset);
                    heap_alloc.heap->slack_memory_allocation_info = mem_scope.record_alloc(heap_alloc.heap->heap->get_remaining() * k_tile_byte_size);
                }
                iter = m_operations.erase(iter);
            }
            else
            {
                due_binds.push_back(&op);
                iter++;
            }
        }
        else
        {
            iter++;
        }
    }

    submit_bind_sparse(due_binds);

    for (operation* op : due_binds)
    {
        auto iter = std::find_if(m_operations.begin(), m_operations.end(), [op](const operation& other) { return &other == op; });
        if (iter != m_operations.end())
        {
            m_operations.erase(iter);
        }
    }
}

void vulkan_ri_tile_manager::begin_frame()
{
    std::scoped_lock lock(m_mutex);

    m_frame_index = m_renderer.get_frame_index();

    perform_operations(m_frame_index);
}

}; // namespace ws
