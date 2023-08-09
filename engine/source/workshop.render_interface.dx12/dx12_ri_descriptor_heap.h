// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/containers/memory_heap.h"
#include "workshop.core/memory/memory_tracker.h"


#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a super simple descriptor heap for dx12.
// ================================================================================================
class dx12_ri_descriptor_heap
{
public:
    struct allocation
    {
    public:
        allocation() = default;
        allocation(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle, size_t heap_start_index, size_t increment, size_t size);

        D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(size_t index);
        D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(size_t index);
        size_t size();
        size_t get_heap_start_index();

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_first_cpu_handle;
        D3D12_GPU_DESCRIPTOR_HANDLE m_first_gpu_handle;
        size_t m_size = 0;
        size_t m_increment = 0;
        size_t m_heap_start_index = 0;

    };

public:
    dx12_ri_descriptor_heap(dx12_render_interface& renderer, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, size_t count);
    virtual ~dx12_ri_descriptor_heap();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    allocation allocate(size_t count = 1);
    void free(allocation handle);

    ID3D12DescriptorHeap* get_resource();

private:
    std::mutex m_mutex;

    dx12_render_interface& m_renderer;
    D3D12_DESCRIPTOR_HEAP_TYPE m_heap_type;
    size_t m_count;

    std::unique_ptr<memory_allocation> m_memory_allocation_info = nullptr;

    UINT m_descriptor_increment = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap = nullptr;

    std::unique_ptr<memory_heap> m_allocation_heap;

};

}; // namespace ws
