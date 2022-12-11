// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_descriptor_heap.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.windowing/window.h"

namespace ws {

dx12_ri_descriptor_heap::dx12_ri_descriptor_heap(dx12_render_interface& renderer, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, size_t count)
    : m_renderer(renderer)
    , m_heap_type(heap_type)
    , m_count(count)
{
}

dx12_ri_descriptor_heap::~dx12_ri_descriptor_heap()
{
    SafeRelease(m_heap);
}

result<void> dx12_ri_descriptor_heap::create_resources()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = static_cast<UINT>(m_count);
    desc.Type = m_heap_type;

    HRESULT hr = m_renderer.get_device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateDescriptorHeap failed with error 0x%08x.", hr);
        return false;
    }

    UINT descriptor_size = m_renderer.get_device()->GetDescriptorHandleIncrementSize(m_heap_type);

    m_free_list.resize(m_count);

    D3D12_CPU_DESCRIPTOR_HANDLE heap_start(m_heap->GetCPUDescriptorHandleForHeapStart());
    for (size_t i = 0; i < m_count; i++)
    {
        m_free_list[i].ptr = reinterpret_cast<SIZE_T>(reinterpret_cast<char*>(heap_start.ptr) + (i * descriptor_size));
    }

    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE dx12_ri_descriptor_heap::allocate()
{
    if (m_free_list.empty())
    {
        db_fatal(render_interface, "DX12 descriptor heap ran out of entries.");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_free_list.back();
    m_free_list.pop_back();

    return handle;
}

void dx12_ri_descriptor_heap::free(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    m_free_list.push_back(handle);
}

}; // namespace ws
