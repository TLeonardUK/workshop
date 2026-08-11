// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_upload_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"
#include "workshop.render_interface.vulkan/vulkan_ri_texture.h"
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_ri_staging_buffer.h"
#include "workshop.core/statistics/statistics_manager.h"
#include "workshop.core/math/math.h"

#include <cstring>

namespace ws {

vulkan_ri_upload_manager::vulkan_ri_upload_manager(vulkan_render_interface& renderer)
    : m_renderer(renderer)
{
    m_stats_render_bytes_uploaded = statistics_manager::get().find_or_create_channel("render/bytes uploaded", 1.0, statistics_commit_point::end_of_render);
}

vulkan_ri_upload_manager::~vulkan_ri_upload_manager()
{
    for (std::unique_ptr<heap_state>& state : m_heaps)
    {
        vkUnmapMemory(m_renderer.get_device(), state->memory);
        vkDestroyBuffer(m_renderer.get_device(), state->handle, nullptr);
        vkFreeMemory(m_renderer.get_device(), state->memory, nullptr);
    }
    m_heaps.clear();
}

void vulkan_ri_upload_manager::allocate_new_heap(size_t minimum_size)
{
    memory_scope mem_scope(memory_type::rendering__vram__upload_heap);

    std::unique_ptr<heap_state> state = std::make_unique<heap_state>();
    state->size = math::round_up_multiple(minimum_size, k_heap_granularity);

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = state->size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult vk_result = vkCreateBuffer(m_renderer.get_device(), &buffer_create_info, nullptr, &state->handle);
    m_renderer.assert_result(vk_result, "vkCreateBuffer");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(m_renderer.get_device(), state->handle, &memory_requirements);

    result<uint32_t> memory_type_index = m_renderer.find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    db_assert(memory_type_index);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = memory_type_index.get_result();

    vk_result = vkAllocateMemory(m_renderer.get_device(), &allocate_info, nullptr, &state->memory);
    m_renderer.assert_result(vk_result, "vkAllocateMemory");

    vk_result = vkBindBufferMemory(m_renderer.get_device(), state->handle, state->memory, 0);
    m_renderer.assert_result(vk_result, "vkBindBufferMemory");

    vk_result = vkMapMemory(m_renderer.get_device(), state->memory, 0, state->size, 0, reinterpret_cast<void**>(&state->start_ptr));
    m_renderer.assert_result(vk_result, "vkMapMemory");

    state->heap = std::make_unique<memory_heap>(state->size);

    m_heaps.push_back(std::move(state));
}

result<void> vulkan_ri_upload_manager::create_resources()
{
    allocate_new_heap(k_heap_granularity);
    return true;
}

vulkan_ri_upload_manager::upload_state vulkan_ri_upload_manager::allocate_upload(size_t size, size_t alignment)
{
    std::scoped_lock lock(m_pending_upload_mutex);

    upload_state state;

    while (true)
    {
        for (std::unique_ptr<heap_state>& heap : m_heaps)
        {
            if (heap->heap->alloc(size, alignment, state.heap_offset))
            {
                heap->last_allocation_frame = m_frame_index;

                state.freed_frame_index = std::numeric_limits<size_t>::max();
                state.heap_size = size;
                state.heap = heap.get();
                break;
            }
        }

        if (!state.heap)
        {
            allocate_new_heap(size);
        }
        else
        {
            break;
        }
    }

    return state;
}

void vulkan_ri_upload_manager::queue_upload(upload_state state)
{
    std::scoped_lock lock(m_pending_upload_mutex);
    state.queued_frame_index = m_frame_index;
    m_pending_uploads.push_back(state);
}

void vulkan_ri_upload_manager::queue_transition(VkImage image, VkImageAspectFlags aspect, ri_resource_state source_state, ri_resource_state destination_state)
{
    std::scoped_lock lock(m_pending_upload_mutex);
    m_pending_transitions.push_back({ image, aspect, source_state, destination_state });
}

void vulkan_ri_upload_manager::upload(vulkan_ri_texture& texture, std::span<uint8_t> data)
{
    memory_scope scope(memory_type::rendering__upload_heap, memory_scope::k_ignore_asset);

    size_t face_count = texture.get_depth();
    size_t mip_count = texture.get_mip_levels();
    size_t array_count = 1;
    if (texture.get_dimensions() == ri_texture_dimension::texture_cube)
    {
        face_count = 6;
        array_count = 6;
    }

    upload_state state = allocate_upload(data.size(), 16);
    state.image = texture.get_image();
    state.image_aspect = texture.get_aspect_mask();
    // Same VK_IMAGE_LAYOUT_UNDEFINED caveat as vulkan_ri_command_list::barrier - a texture's
    // VkImage always starts life in UNDEFINED regardless of what get_initial_state()'s
    // conceptual "common" state is, so the first-ever transition (here, into TRANSFER_DST_OPTIMAL
    // for the upload copy below) must come from the real UNDEFINED, not get_initial_state().
    // resource_steady_state is always the real resolved state though - it's what the texture
    // gets transitioned back to after the upload, and must never be left as UNDEFINED.
    state.resource_steady_state = texture.get_initial_state();
    state.resource_initial_state = texture.has_undefined_layout() ? ri_resource_state::initial : state.resource_steady_state;
    texture.clear_undefined_layout();
    state.name = texture.get_debug_name();

    memcpy(state.heap->start_ptr + state.heap_offset, data.data(), data.size());

    std::vector<VkBufferImageCopy2> regions;
    regions.reserve(array_count * mip_count);

    for (size_t array_index = 0; array_index < array_count; array_index++)
    {
        for (size_t mip_index = 0; mip_index < mip_count; mip_index++)
        {
            size_t mip_offset = 0;
            size_t mip_size = 0;
            texture.calculate_linear_data_mip_range(array_index, mip_index, mip_offset, mip_size);

            uint32_t mip_width = std::max(size_t{1}, texture.get_width() >> mip_index);
            uint32_t mip_height = std::max(size_t{1}, texture.get_height() >> mip_index);

            VkBufferImageCopy2 region = {};
            region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
            region.bufferOffset = state.heap_offset + mip_offset;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = state.image_aspect;
            region.imageSubresource.mipLevel = static_cast<uint32_t>(mip_index);
            region.imageSubresource.baseArrayLayer = static_cast<uint32_t>(array_index);
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { mip_width, mip_height, 1 };

            regions.push_back(region);
        }
    }

    state.build_command_list = [image = state.image, buffer = state.heap->handle, regions](vulkan_ri_command_list& list)
    {
        VkCopyBufferToImageInfo2 copy_info = {};
        copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
        copy_info.srcBuffer = buffer;
        copy_info.dstImage = image;
        copy_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy_info.regionCount = static_cast<uint32_t>(regions.size());
        copy_info.pRegions = regions.data();

        vkCmdCopyBufferToImage2(list.get_command_buffer(), &copy_info);
    };

    queue_upload(state);
}

void vulkan_ri_upload_manager::upload(vulkan_ri_texture& texture, size_t array_index, size_t mip_index, std::span<uint8_t> data)
{
    memory_scope scope(memory_type::rendering__upload_heap, memory_scope::k_ignore_asset);

    upload_state state = allocate_upload(data.size(), 16);
    state.image = texture.get_image();
    state.image_aspect = texture.get_aspect_mask();
    // See the equivalent comment in the other upload() overload above.
    state.resource_steady_state = texture.get_initial_state();
    state.resource_initial_state = texture.has_undefined_layout() ? ri_resource_state::initial : state.resource_steady_state;
    texture.clear_undefined_layout();
    state.name = texture.get_debug_name();

    memcpy(state.heap->start_ptr + state.heap_offset, data.data(), data.size());

    uint32_t mip_width = static_cast<uint32_t>(std::max(size_t{1}, texture.get_width() >> mip_index));
    uint32_t mip_height = static_cast<uint32_t>(std::max(size_t{1}, texture.get_height() >> mip_index));

    VkBufferImageCopy2 region = {};
    region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
    region.bufferOffset = state.heap_offset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = state.image_aspect;
    region.imageSubresource.mipLevel = static_cast<uint32_t>(mip_index);
    region.imageSubresource.baseArrayLayer = static_cast<uint32_t>(array_index);
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { mip_width, mip_height, 1 };

    state.build_command_list = [image = state.image, buffer = state.heap->handle, region](vulkan_ri_command_list& list)
    {
        VkCopyBufferToImageInfo2 copy_info = {};
        copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
        copy_info.srcBuffer = buffer;
        copy_info.dstImage = image;
        copy_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy_info.regionCount = 1;
        copy_info.pRegions = &region;

        vkCmdCopyBufferToImage2(list.get_command_buffer(), &copy_info);
    };

    queue_upload(state);
}

void vulkan_ri_upload_manager::upload(vulkan_ri_texture& texture, size_t array_index, size_t mip_index, vulkan_ri_staging_buffer& data_buffer)
{
    // Async pre-staged texture uploads are not yet implemented (vulkan_ri_staging_buffer is
    // still a stub) - fall through to a synchronous upload of whatever data it holds isn't
    // possible without that class exposing its data, so this path is a no-op for now.
    db_error(render_interface, "vulkan_ri_upload_manager::upload(texture&, staging_buffer&) is not yet implemented.");
}

void vulkan_ri_upload_manager::upload(vulkan_ri_buffer& buffer, std::span<uint8_t> data, size_t offset)
{
    // vulkan_ri_buffer is always persistently host-mapped, so there's no need to route
    // through the upload heap ring at all - just write directly.
    void* dest = buffer.map(offset, data.size());
    memcpy(dest, data.data(), data.size());
    buffer.unmap(dest);
}

void vulkan_ri_upload_manager::begin_frame()
{
    std::scoped_lock lock(m_pending_upload_mutex);

    m_frame_index = m_renderer.get_frame_index();
}

size_t vulkan_ri_upload_manager::get_heap_size()
{
    size_t total = 0;

    for (size_t i = 1; i < m_heaps.size(); i++)
    {
        total += m_heaps[i]->size;
    }

    return total;
}

void vulkan_ri_upload_manager::free_uploads()
{
    size_t pipeline_depth = m_renderer.get_pipeline_depth();
    if (m_frame_index < pipeline_depth)
    {
        return;
    }

    size_t free_frame_index = m_frame_index - pipeline_depth;

    for (size_t i = 0; i < m_pending_free.size(); /* empty */)
    {
        upload_state& entry = m_pending_free[i];

        if (entry.freed_frame_index <= free_frame_index)
        {
            entry.heap->heap->free(entry.heap_offset);

            m_pending_free[i] = m_pending_free.back();
            m_pending_free.resize(m_pending_free.size() - 1);
        }
        else
        {
            i++;
        }
    }

    if (m_heaps.size() > 1 && get_heap_size() > k_persist_heap_memory)
    {
        for (size_t i = 1; i < m_heaps.size(); /* empty */)
        {
            heap_state* heap = m_heaps[i].get();

            if (heap->heap->empty())
            {
                vkUnmapMemory(m_renderer.get_device(), heap->memory);
                vkDestroyBuffer(m_renderer.get_device(), heap->handle, nullptr);
                vkFreeMemory(m_renderer.get_device(), heap->memory, nullptr);

                m_heaps.erase(m_heaps.begin() + i);
            }
            else
            {
                i++;
            }
        }
    }
}

void vulkan_ri_upload_manager::perform_transitions()
{
    std::vector<transition_state> transitions;
    {
        std::scoped_lock lock(m_pending_upload_mutex);
        transitions = std::move(m_pending_transitions);
        m_pending_transitions.clear();
    }

    if (transitions.empty())
    {
        return;
    }

    vulkan_ri_command_queue& graphics_queue = static_cast<vulkan_ri_command_queue&>(m_renderer.get_graphics_queue());

    vulkan_ri_command_list& list = static_cast<vulkan_ri_command_list&>(graphics_queue.alloc_command_list());
    list.open();
    for (transition_state& state : transitions)
    {
        list.barrier(state.image, state.source_state, state.destination_state, state.image_aspect);
    }
    list.close();
    graphics_queue.execute(list);
}

void vulkan_ri_upload_manager::perform_uploads()
{
    vulkan_ri_command_queue& graphics_queue = static_cast<vulkan_ri_command_queue&>(m_renderer.get_graphics_queue());

    size_t total_bytes = 0;

    std::vector<upload_state> uploads;
    {
        std::scoped_lock lock(m_pending_upload_mutex);
        uploads = m_pending_uploads;
        m_pending_uploads.clear();
    }

    if (uploads.empty())
    {
        m_stats_render_bytes_uploaded->submit(0.0);
        return;
    }

    std::vector<upload_state> unique_resources;
    for (upload_state& upload : uploads)
    {
        auto iter = std::find_if(unique_resources.begin(), unique_resources.end(), [&upload](upload_state& b) {
            return b.image == upload.image;
        });
        if (iter == unique_resources.end())
        {
            unique_resources.push_back(upload);
        }
    }

    {
        vulkan_ri_command_list& transition_list = static_cast<vulkan_ri_command_list&>(graphics_queue.alloc_command_list());
        transition_list.open();
        for (upload_state& state : unique_resources)
        {
            transition_list.barrier(state.image, state.resource_initial_state, ri_resource_state::copy_dest, state.image_aspect);
        }
        transition_list.close();
        graphics_queue.execute(transition_list);
    }

    constexpr size_t k_block_size = 32;

    for (size_t i = 0; i < uploads.size(); i += k_block_size)
    {
        vulkan_ri_command_list& list = static_cast<vulkan_ri_command_list&>(graphics_queue.alloc_command_list());
        list.open();

        for (size_t j = i; j < i + k_block_size && j < uploads.size(); j++)
        {
            upload_state& state = uploads[j];

            state.build_command_list(list);

            state.freed_frame_index = m_frame_index;
            {
                std::scoped_lock lock(m_pending_upload_mutex);
                m_pending_free.push_back(state);
            }

            total_bytes += state.heap_size;
        }

        list.close();
        graphics_queue.execute(list);
    }

    {
        vulkan_ri_command_list& transition_list = static_cast<vulkan_ri_command_list&>(graphics_queue.alloc_command_list());
        transition_list.open();
        for (upload_state& state : unique_resources)
        {
            transition_list.barrier(state.image, ri_resource_state::copy_dest, state.resource_steady_state, state.image_aspect);
        }
        transition_list.close();
        graphics_queue.execute(transition_list);
    }

    m_stats_render_bytes_uploaded->submit(static_cast<double>(total_bytes));
}

void vulkan_ri_upload_manager::flush()
{
    std::scoped_lock lock(m_pending_upload_mutex);

    perform_transitions();
    perform_uploads();
    free_uploads();
}

}; // namespace ws
