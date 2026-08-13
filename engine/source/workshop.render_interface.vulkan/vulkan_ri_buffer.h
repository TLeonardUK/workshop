// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"
#include "workshop.render_interface.vulkan/vulkan_ri_small_buffer_allocator.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/memory/memory_tracker.h"

#include <string>
#include <mutex>

namespace ws {

class vulkan_render_interface;

// ================================================================================================
//  Implementation of a gpu buffer using Vulkan.
// ================================================================================================
class vulkan_ri_buffer : public ri_buffer
{
public:
    vulkan_ri_buffer(vulkan_render_interface& renderer, const char* debug_name, const ri_buffer::create_params& params);
    virtual ~vulkan_ri_buffer();

    result<void> create_resources();

    virtual size_t get_element_count() override;
    virtual size_t get_element_size() override;

    virtual const char* get_debug_name() override;

    virtual ri_resource_state get_initial_state() override;

    virtual void* map(size_t offset, size_t size) override;
    virtual void unmap(void* pointer) override;

    bool is_small_buffer();
    size_t get_buffer_offset();

    VkDeviceAddress get_gpu_address();

    vulkan_ri_descriptor_table::allocation get_srv() const;
    vulkan_ri_descriptor_table::allocation get_uav() const;

    vulkan_ri_small_buffer_allocator::handle get_small_buffer_allocation() const;

    VkBuffer get_buffer();

private:
    result<void> create_exclusive_buffer();

private:
    vulkan_render_interface& m_renderer;
    std::string m_debug_name;
    ri_buffer::create_params m_create_params;

    std::unique_ptr<memory_allocation> m_memory_allocation_info = nullptr;

    ri_resource_state m_common_state = ri_resource_state::initial;

    VkBuffer m_handle = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;

    ri_descriptor_table m_srv_table = ri_descriptor_table::buffer;
    ri_descriptor_table m_uav_table = ri_descriptor_table::rwbuffer;

    vulkan_ri_descriptor_table::allocation m_srv;
    vulkan_ri_descriptor_table::allocation m_uav;

    bool m_is_small_buffer = false;
    vulkan_ri_small_buffer_allocator::handle m_small_buffer_allocation;

    struct mapped_buffer
    {
        size_t offset;
        size_t size;
        std::vector<uint8_t> data;
        void* ptr;
    };

    std::mutex m_buffers_mutex;
    std::vector<mapped_buffer> m_buffers;

    std::atomic<size_t> m_map_counter;
    void* m_mapped_ptr = nullptr;

};

}; // namespace ws
