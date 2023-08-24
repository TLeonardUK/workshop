// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_query_manager.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_command_list.h"

namespace ws {

dx12_ri_query_manager::dx12_ri_query_manager(dx12_render_interface& renderer, size_t max_queries)
    : m_renderer(renderer)
    , m_max_queries(max_queries)
{
}

dx12_ri_query_manager::~dx12_ri_query_manager()
{
}

result<void> dx12_ri_query_manager::create_resources()
{
    memory_scope mem_scope(memory_type::rendering__vram__queries);

    m_pipeline_depth = m_renderer.get_pipeline_depth();
    m_query_slots = m_max_queries * 2;
    m_read_back_times.resize(m_query_slots);
    m_query_info.resize(m_max_queries);
    for (size_t i = 0; i < m_max_queries; i++)
    {
        m_query_info[i].started_frame = std::numeric_limits<size_t>::max();
        m_free_queries.push_back((m_max_queries - 1) - i);
    }

    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type = D3D12_HEAP_TYPE_READBACK;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask = 0;
    heap_properties.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = sizeof(uint64_t) * m_query_slots * m_renderer.get_pipeline_depth();
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = m_renderer.get_device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_read_back_buffer)
    );
    if (FAILED(hr))
    {
        db_fatal(render_interface, "CreateCommittedResource failed with error 0x%08x when creating query readback buffer.", hr);
        return false;
    }

    // Record the memory allocation.
    D3D12_RESOURCE_ALLOCATION_INFO info = m_renderer.get_device()->GetResourceAllocationInfo(0, 1, &desc);
    m_memory_allocation_info = mem_scope.record_alloc(info.SizeInBytes);

    D3D12_QUERY_HEAP_DESC query_heap_desc;
    query_heap_desc.Count = static_cast<UINT>(m_query_slots);
    query_heap_desc.NodeMask = 1;
    query_heap_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    hr = m_renderer.get_device()->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(&m_query_heap));
    if (FAILED(hr))
    {
        db_fatal(render_interface, "CreateCommittedResource failed with error 0x%08x when creating query heap.", hr);
        return false;
    }

    m_read_back_buffer->SetName(L"Query Readback Buffer");
    m_query_heap->SetName(L"Query Heap");

    return true;
}

dx12_ri_query_manager::query_id dx12_ri_query_manager::new_query(ri_query_type type)
{
    std::scoped_lock lock(m_mutex);

    if (m_free_queries.empty())
    {
        db_error(render_interface, "Ran out of gpu queries. Failed to allocate new timer, results may be unexpected.");
        return k_invalid_query_id;
    }

    query_id timer = m_free_queries.back();
    m_free_queries.pop_back();

    m_query_info[timer].type = type;
    m_query_info[timer].started_frame = std::numeric_limits<size_t>::max();

    return timer;
}

void dx12_ri_query_manager::delete_query(query_id id)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return;
    }

    m_free_queries.push_back(id);

    m_query_info[id].started_frame = std::numeric_limits<size_t>::max();
}

bool dx12_ri_query_manager::are_results_ready(query_id id)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return true;
    }

    return m_renderer.get_frame_index() > m_query_info[id].started_frame + m_pipeline_depth;
}

double dx12_ri_query_manager::get_result(query_id id)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return 0.0;
    }

    query_info& info = m_query_info[id];

    switch (info.type)
    {
    case ri_query_type::time:
        {
            uint64_t start = m_read_back_times[id * 2];
            uint64_t end = m_read_back_times[id * 2 + 1];

            if (end <= start)
            {
                return 0.0f;
            }

            return float(double(end - start) * m_timestamp_frequency_inv);
        }
    }

    return 0.0f;
}

void dx12_ri_query_manager::start_query(query_id id, ID3D12GraphicsCommandList* list)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return;
    }

    list->EndQuery(m_query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, static_cast<UINT>(id) * 2);
}

void dx12_ri_query_manager::end_query(query_id id, ID3D12GraphicsCommandList* list)
{
    std::scoped_lock lock(m_mutex);

    if (id == k_invalid_query_id)
    {
        return;
    }

    list->EndQuery(m_query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, static_cast<UINT>(id) * 2 + 1);

    // If this is the first time using the query record the frame so we can know when
    // results are first available.
    if (m_query_info[id].started_frame == std::numeric_limits<size_t>::max())
    {
        m_query_info[id].started_frame = m_renderer.get_frame_index();
    }
}

void dx12_ri_query_manager::begin_frame()
{
    std::scoped_lock lock(m_mutex);

    profile_marker(profile_colors::render, "query readback");

    size_t resolve_base_offset = m_resolve_frame_index * m_query_slots * sizeof(uint64_t);
    dx12_ri_command_queue& queue = static_cast<dx12_ri_command_queue&>(m_renderer.get_graphics_queue());

    // Execute a command list to resolve queries for this frames timers.
    dx12_ri_command_list& list = static_cast<dx12_ri_command_list&>(queue.alloc_command_list());
    list.open();
    list.get_dx_command_list()->ResolveQueryData(m_query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, static_cast<UINT>(m_query_slots), m_read_back_buffer.Get(), resolve_base_offset);
    list.close();
    m_renderer.get_graphics_queue().execute(list);

    // Read back timers from frame finished pipeline depth ago.
    size_t read_back_index = (m_resolve_frame_index + 1) % m_pipeline_depth;
    size_t read_back_offset = read_back_index * m_query_slots * sizeof(uint64_t);
    D3D12_RANGE data_range = {
        read_back_offset,
        read_back_offset + m_query_slots * sizeof(uint64_t)
    };

    uint64_t* timing_data;
    HRESULT hr = m_read_back_buffer->Map(0, &data_range, reinterpret_cast<void**>(&timing_data));
    if (FAILED(hr))
    {
        db_error(renderer, "Failed to read back query buffer with error 0x%08x", hr);
        return;
    }

    memcpy(m_read_back_times.data(), timing_data, sizeof(uint64_t) * m_query_slots);
    m_read_back_buffer->Unmap(0, nullptr);

    m_resolve_frame_index = read_back_index;

    // Store the timestamp frequency so we can turn the time stamps into something useable.
    hr = queue.get_queue()->GetTimestampFrequency(&m_timestamp_frequency);
    if (FAILED(hr))
    {
        db_error(renderer, "Failed to get the direct queues timestamp frequency with error 0x%08x", hr);
        return;
    }
    m_timestamp_frequency_inv = 1.0 / double(m_timestamp_frequency);
}

}; // namespace ws
