// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"

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
    dx12_ri_descriptor_heap(dx12_render_interface& renderer, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, size_t count);
    virtual ~dx12_ri_descriptor_heap();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    D3D12_CPU_DESCRIPTOR_HANDLE allocate();
    void free(D3D12_CPU_DESCRIPTOR_HANDLE handle);

private:
    dx12_render_interface& m_renderer;
    D3D12_DESCRIPTOR_HEAP_TYPE m_heap_type;
    size_t m_count;

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_free_list;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap = nullptr;

};

}; // namespace ws
