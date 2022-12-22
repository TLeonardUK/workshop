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
    m_allocation_heap = std::make_unique<memory_heap>(m_count);
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

    // Sampler and SRV heaps are visible as they are used for bindless descriptor tables.
    if (m_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER || m_heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    HRESULT hr = m_renderer.get_device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateDescriptorHeap failed with error 0x%08x.", hr);
        return false;
    }

    m_descriptor_increment = m_renderer.get_device()->GetDescriptorHandleIncrementSize(m_heap_type);

    return true;
}

dx12_ri_descriptor_heap::allocation dx12_ri_descriptor_heap::allocate(size_t count)
{
    std::scoped_lock lock(m_mutex);

    size_t start_index = 0;
    if (!m_allocation_heap->alloc(count, 0, start_index))
    {
        db_fatal(render_interface, "Descriptor heap ran out of descriptors while trying to allocate %zi.", count);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE heap_start_cpu(m_heap->GetCPUDescriptorHandleForHeapStart());
    D3D12_GPU_DESCRIPTOR_HANDLE heap_start_gpu(m_heap->GetGPUDescriptorHandleForHeapStart());

    D3D12_CPU_DESCRIPTOR_HANDLE start_cpu;
    start_cpu.ptr = reinterpret_cast<SIZE_T>(reinterpret_cast<char*>(heap_start_cpu.ptr) + (start_index * m_descriptor_increment));

    D3D12_GPU_DESCRIPTOR_HANDLE start_gpu;
    start_gpu.ptr = reinterpret_cast<SIZE_T>(reinterpret_cast<char*>(heap_start_gpu.ptr) + (start_index * m_descriptor_increment));

    return allocation(start_cpu, start_gpu, start_index, m_descriptor_increment, count);
}

void dx12_ri_descriptor_heap::free(allocation handle)
{
    std::scoped_lock lock(m_mutex);

    m_allocation_heap->free(handle.get_heap_start_index());
}

ID3D12DescriptorHeap* dx12_ri_descriptor_heap::get_resource()
{
    return m_heap.Get();
}


dx12_ri_descriptor_heap::allocation::allocation(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle, size_t heap_start_index, size_t increment, size_t size)
    : m_first_cpu_handle(cpu_handle)
    , m_first_gpu_handle(gpu_handle)
    , m_heap_start_index(heap_start_index)
    , m_increment(increment)
    , m_size(size)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE dx12_ri_descriptor_heap::allocation::get_cpu_handle(size_t index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    handle.ptr = reinterpret_cast<SIZE_T>(reinterpret_cast<char*>(m_first_cpu_handle.ptr) + (index * m_increment));
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE dx12_ri_descriptor_heap::allocation::get_gpu_handle(size_t index)
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle;
    handle.ptr = reinterpret_cast<SIZE_T>(reinterpret_cast<char*>(m_first_gpu_handle.ptr) + (index * m_increment));
    return handle;
}

size_t dx12_ri_descriptor_heap::allocation::size()
{
    return m_size;
}

size_t dx12_ri_descriptor_heap::allocation::get_heap_start_index()
{
    return m_heap_start_index;
}

}; // namespace ws
