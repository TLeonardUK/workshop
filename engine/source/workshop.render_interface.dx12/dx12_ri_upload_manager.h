// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/containers/memory_heap.h"
#include "workshop.core/memory/memory_tracker.h"

#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.render_interface/ri_types.h"

#include <array>
#include <span>
#include <functional>

// Performs a lot of additional debugging checks for the upload manager.
//#define UPLOAD_DEBUG 

namespace ws {

class engine;
class dx12_render_interface;
class dx12_ri_texture;
class dx12_ri_buffer;
class dx12_ri_command_list;
class statistics_channel;
class ri_fence;

// ================================================================================================
//  Handles copying CPU data to GPU memory.
// ================================================================================================
class dx12_ri_upload_manager
{
public:
    dx12_ri_upload_manager(dx12_render_interface& renderer);
    virtual ~dx12_ri_upload_manager();

    result<void> create_resources();

    void upload(dx12_ri_texture& source, const std::span<uint8_t>& data);
    void upload(dx12_ri_texture& source, size_t array_index, size_t mip_index, const std::span<uint8_t>& data);
    void upload(dx12_ri_buffer& source, const std::span<uint8_t>& data, size_t offset);

    void new_frame(size_t index);

private:
    using build_command_list_callback_t = std::function<void(dx12_ri_command_list& list)>;

    struct heap_state
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> handle = nullptr;
        uint8_t* start_ptr = nullptr;

        std::unique_ptr<memory_heap> memory_heap;
        size_t size = 0;

        size_t last_allocation_frame = 0;

        std::unique_ptr<memory_allocation> memory_allocation_info;
    };

    struct upload_state
    {
        size_t freed_frame_index;
        size_t queued_frame_index;
        size_t heap_offset;
        size_t heap_size;

        heap_state* heap = nullptr;

        ID3D12Resource* resource = nullptr;
        ri_resource_state resource_initial_state;

        build_command_list_callback_t build_command_list = nullptr;

        const char* name = nullptr;
    };

    // Granularity of heap size. The actual heap size is based on the size of the data to be uploaded.
    static constexpr size_t k_heap_granularity = 32 * 1024 * 1024;

private:
    void allocate_new_heap(size_t minimum_size);
    
    upload_state allocate_upload(size_t size, size_t alignment);
    void queue_upload(upload_state state);

    void perform_uploads();
    void free_uploads();

private:
    std::recursive_mutex m_pending_upload_mutex;
    std::vector<upload_state> m_pending_uploads;
    std::vector<upload_state> m_pending_free;
    
    dx12_render_interface& m_renderer;
    std::unique_ptr<ri_fence> m_graphics_queue_fence;
    std::unique_ptr<ri_fence> m_copy_queue_fence;

    size_t m_frame_index = 0;

    //Microsoft::WRL::ComPtr<ID3D12Resource> m_heap_handle = nullptr;
    //uint8_t* m_upload_heap_ptr = nullptr;
    //std::unique_ptr<memory_heap> m_upload_heap;

    std::vector<std::unique_ptr<heap_state>> m_heaps;

    statistics_channel* m_stats_render_bytes_uploaded = nullptr;

};

}; // namespace ws
