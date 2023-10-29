// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_upload_manager.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"
#include "workshop.core/memory/memory_tracker.h"

namespace ws {

dx12_ri_buffer::dx12_ri_buffer(dx12_render_interface& renderer, const char* debug_name, const ri_buffer::create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
{
}

dx12_ri_buffer::~dx12_ri_buffer()
{
    m_renderer.defer_delete([renderer = &m_renderer, handle = m_handle, srv = m_srv, uav = m_uav, srv_table = m_srv_table, uav_table = m_uav_table]()
    {
        if (srv.is_valid())
        {
            renderer->get_descriptor_table(srv_table).free(srv);
        }
        if (uav.is_valid())
        {
            renderer->get_descriptor_table(uav_table).free(uav);
        }
        //CheckedRelease(handle);    
    });
}

bool dx12_ri_buffer::is_small_buffer()
{
    return m_is_small_buffer;
}

size_t dx12_ri_buffer::get_buffer_offset()
{
    if (m_is_small_buffer)
    {
        return m_small_buffer_allocation.offset;
    }
    else
    {
        return 0;
    }
}

result<void> dx12_ri_buffer::create_exclusive_buffer()
{
    memory_type mem_type;
    switch (m_create_params.usage)
    {
    case ri_buffer_usage::index_buffer:
    {
        mem_type = memory_type::rendering__vram__index_buffer;
        break;
    }
    case ri_buffer_usage::vertex_buffer:
    {
        mem_type = memory_type::rendering__vram__vertex_buffer;
        break;
    }    
    case ri_buffer_usage::raytracing_as:
    case ri_buffer_usage::raytracing_as_instance_data:
    case ri_buffer_usage::raytracing_as_scratch:
    case ri_buffer_usage::raytracing_shader_binding_table:
    {
        mem_type = memory_type::rendering__vram__raytracing_buffer;
        break;
    }
    case ri_buffer_usage::generic:
    default:
    {
        mem_type = memory_type::rendering__vram__generic_buffer;
        break;
    }
    }

    memory_scope mem_scope(mem_type, string_hash::empty, string_hash(m_debug_name));

    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask = 0;
    heap_properties.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC desc;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.DepthOrArraySize = 1;
    desc.Width = m_create_params.element_count * m_create_params.element_size;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    if (m_create_params.usage == ri_buffer_usage::generic ||
        m_create_params.usage == ri_buffer_usage::raytracing_as_instance_data ||
        m_create_params.usage == ri_buffer_usage::raytracing_as_scratch ||
        m_create_params.usage == ri_buffer_usage::raytracing_as ||
        m_create_params.usage == ri_buffer_usage::raytracing_shader_binding_table)
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    HRESULT hr = m_renderer.get_device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        ri_to_dx12(m_common_state),
        nullptr,
        IID_PPV_ARGS(&m_handle)
    );
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateCommittedResource failed with error 0x%08x.", hr);
        return false;
    }

    // Set a debug name.
    m_handle->SetName(widen_string(m_debug_name).c_str());

    // Record the memory allocation.
    D3D12_RESOURCE_ALLOCATION_INFO info = m_renderer.get_device()->GetResourceAllocationInfo(0, 1, &desc);
    m_memory_allocation_info = mem_scope.record_alloc(info.SizeInBytes);

    return true;
}

result<void> dx12_ri_buffer::create_resources()
{
    // Either create an exclusive buffer or sub-allocate into the small buffer allocator.
    dx12_ri_small_buffer_allocator& small_allocator = m_renderer.get_small_buffer_allocator();
    size_t total_size = m_create_params.element_size * m_create_params.element_count;
    m_is_small_buffer = (total_size < small_allocator.get_max_size());

    m_uav_table = ri_descriptor_table::rwbuffer;
    m_srv_table = ri_descriptor_table::buffer;

    switch (m_create_params.usage)
    {
    case ri_buffer_usage::index_buffer:
        {
            m_common_state = ri_resource_state::index_buffer;
            break;
        }
    case ri_buffer_usage::raytracing_as:
        {
            m_common_state = ri_resource_state::raytracing_acceleration_structure;
            m_srv_table = ri_descriptor_table::tlas;
            break;
        }
    case ri_buffer_usage::raytracing_shader_binding_table:
    case ri_buffer_usage::raytracing_as_instance_data:
        {
            m_common_state = ri_resource_state::non_pixel_shader_resource;
            break;
        }
    case ri_buffer_usage::raytracing_as_scratch:
        {
            m_common_state = ri_resource_state::unordered_access;
            break;
        }
    case ri_buffer_usage::vertex_buffer:
        {
            m_common_state = ri_resource_state::pixel_shader_resource;
            break;
        }
    case ri_buffer_usage::generic:
        {
            m_common_state = ri_resource_state::pixel_shader_resource;
            break;
        }
    default:
        {
            m_common_state = ri_resource_state::pixel_shader_resource;
            break;
        }
    }

    if (m_is_small_buffer)
    {
        if (!small_allocator.alloc(total_size, m_create_params.usage, m_small_buffer_allocation))
        {
            return false;
        }
    }
    else
    {
        if (!create_exclusive_buffer())
        {
            return false;
        }
    }

    size_t sub_allocation_offset = 0;
    if (m_is_small_buffer)
    {
        sub_allocation_offset = m_small_buffer_allocation.offset;
    }

    // Upload the linear data if any has been provided.
    if (!m_create_params.linear_data.empty())
    {
        m_renderer.get_upload_manager().upload(
            *this,
            m_create_params.linear_data,
            sub_allocation_offset
        );
    }

    // Create SRV view for buffer.
    D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = {};
    view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (m_create_params.usage == ri_buffer_usage::raytracing_as)
    {
        view_desc.Format = DXGI_FORMAT_UNKNOWN;
        view_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        view_desc.RaytracingAccelerationStructure.Location = get_gpu_address();
    }
    else
    {
        view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        view_desc.Buffer.FirstElement = sub_allocation_offset / 4;
        view_desc.Buffer.NumElements = static_cast<UINT>((m_create_params.element_count * m_create_params.element_size) / 4);
        view_desc.Buffer.StructureByteStride = 0;
        view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    }

    m_srv = m_renderer.get_descriptor_table(m_srv_table).allocate();

    if (m_create_params.usage == ri_buffer_usage::raytracing_as)
    {
        m_renderer.get_device()->CreateShaderResourceView(nullptr, &view_desc, m_srv.cpu_handle);
    }
    else
        m_renderer.get_device()->CreateShaderResourceView(get_resource(), &view_desc, m_srv.cpu_handle);
    {
    }

    // Create a UAV view for the buffer as well incase we need unordered access later.
    if (m_create_params.usage == ri_buffer_usage::generic ||
        m_create_params.usage == ri_buffer_usage::raytracing_as ||
        m_create_params.usage == ri_buffer_usage::raytracing_as_scratch ||
        m_create_params.usage == ri_buffer_usage::raytracing_as_instance_data ||
        m_create_params.usage == ri_buffer_usage::raytracing_shader_binding_table)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_view_desc = {};
        uav_view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        uav_view_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_view_desc.Buffer.FirstElement = sub_allocation_offset / 4;
        uav_view_desc.Buffer.NumElements = static_cast<UINT>((m_create_params.element_count * m_create_params.element_size) / 4);
        uav_view_desc.Buffer.StructureByteStride = 0;
        uav_view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

        m_uav = m_renderer.get_descriptor_table(m_uav_table).allocate();
        m_renderer.get_device()->CreateUnorderedAccessView(get_resource(), nullptr, &uav_view_desc, m_uav.cpu_handle);
    }

    return true;
}

D3D12_GPU_VIRTUAL_ADDRESS dx12_ri_buffer::get_gpu_address()
{
    if (m_is_small_buffer)
    {
        return get_resource()->GetGPUVirtualAddress() + m_small_buffer_allocation.offset;
    }
    else
    {
        return get_resource()->GetGPUVirtualAddress();
    }
}

size_t dx12_ri_buffer::get_element_count()
{
    return m_create_params.element_count;
}

size_t dx12_ri_buffer::get_element_size()
{
    return m_create_params.element_size;
}

const char* dx12_ri_buffer::get_debug_name()
{
    return m_debug_name.c_str();
}

ID3D12Resource* dx12_ri_buffer::get_resource()
{
    if (m_is_small_buffer)
    {
        return static_cast<dx12_ri_buffer*>(m_small_buffer_allocation.buffer)->get_resource();
    }
    else
    {
        return m_handle.Get();
    }
}

ri_resource_state dx12_ri_buffer::get_initial_state()
{
    return m_common_state;
}

dx12_ri_descriptor_table::allocation dx12_ri_buffer::get_srv() const
{
    db_assert(m_srv.is_valid());
    return m_srv;
}

dx12_ri_descriptor_table::allocation dx12_ri_buffer::get_uav() const
{
    db_assert(m_uav.is_valid());
    return m_uav;
}

dx12_ri_small_buffer_allocator::handle dx12_ri_buffer::get_small_buffer_allocation() const
{
    return m_small_buffer_allocation;
}

void* dx12_ri_buffer::map(size_t offset, size_t size)
{
    if (m_is_small_buffer)
    {
        db_assert(offset >= 0 && offset + size <= m_small_buffer_allocation.size);

        return static_cast<dx12_ri_buffer*>(m_small_buffer_allocation.buffer)->map(m_small_buffer_allocation.offset + offset, size);
    }
    else
    {
        std::scoped_lock lock(m_buffers_mutex);

        db_assert(offset >= 0 && offset + size <= (m_create_params.element_count * m_create_params.element_size));

        mapped_buffer& buf = m_buffers.emplace_back();
        buf.offset = offset;
        buf.size = size;
        buf.data.resize(size);

        return buf.data.data();
    }
}

void dx12_ri_buffer::unmap(void* pointer)
{
    if (m_is_small_buffer)
    {
        return static_cast<dx12_ri_buffer*>(m_small_buffer_allocation.buffer)->unmap(pointer);
    }
    else
    {
        std::scoped_lock lock(m_buffers_mutex);

        for (auto iter = m_buffers.begin(); iter != m_buffers.end(); iter++)
        {
            if (iter->data.data() == pointer)
            {
                m_renderer.get_upload_manager().upload(
                    *this,
                    iter->data,
                    iter->offset
                );

                m_buffers.erase(iter);
                return;
            }
        }
    }
}

}; // namespace ws
