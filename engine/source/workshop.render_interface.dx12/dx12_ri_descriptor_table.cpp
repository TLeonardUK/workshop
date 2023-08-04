// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"

namespace ws {

dx12_ri_descriptor_table::dx12_ri_descriptor_table(dx12_render_interface& renderer, ri_descriptor_table table_type)
    : m_renderer(renderer)
    , m_table_type(table_type)
{

    m_size = dx12_render_interface::k_descriptor_table_sizes[(int)m_table_type];

    switch (m_table_type)
    {
        case ri_descriptor_table::texture_1d:
        case ri_descriptor_table::texture_2d:
        case ri_descriptor_table::texture_3d:
        case ri_descriptor_table::texture_cube:
        case ri_descriptor_table::buffer:
        case ri_descriptor_table::rwtexture_2d:
        case ri_descriptor_table::rwbuffer:
        {
            m_heap = &m_renderer.get_srv_descriptor_heap();
            break;
        }
        case ri_descriptor_table::sampler:
        {
            m_heap = &m_renderer.get_sampler_descriptor_heap();
            break;
        }
        case ri_descriptor_table::render_target:
        {
            m_heap = &m_renderer.get_rtv_descriptor_heap();
            break;
        }
        case ri_descriptor_table::depth_stencil:
        {
            m_heap = &m_renderer.get_dsv_descriptor_heap();
            break;
        }
        default:
        {
            db_fatal(renderer, "Unsupported descriptor table being created.");
            break;
        }
    }
}

dx12_ri_descriptor_table::~dx12_ri_descriptor_table()
{
    m_heap->free(m_allocation);    
}

size_t dx12_ri_descriptor_table::get_used_count()
{
    std::scoped_lock lock(m_mutex);

    return m_size - m_free_list.size();
}

size_t dx12_ri_descriptor_table::get_total_count()
{
    return m_size;
}

result<void> dx12_ri_descriptor_table::create_resources()
{
    m_allocation = m_heap->allocate(m_size);

    m_free_list.resize(m_size);
    for (size_t i = 0; i < m_size; i++)
    {
        m_free_list[i] = i;
    }

    return true;
}

dx12_ri_descriptor_table::allocation dx12_ri_descriptor_table::allocate()
{
    std::scoped_lock lock(m_mutex);

    allocation result {};

    if (m_free_list.size() == 0)
    {
        db_fatal(renderer, "Descriptor table ran out of descriptors to allocate!");
    }

    result.valid = true;
    result.index = m_free_list.back();
    result.cpu_handle = m_allocation.get_cpu_handle(result.index);
    result.gpu_handle = m_allocation.get_gpu_handle(result.index);

    m_free_list.pop_back();

    return result;
}

void dx12_ri_descriptor_table::free(allocation handle)
{
    std::scoped_lock lock(m_mutex);

    m_free_list.push_back(handle.index);
}

dx12_ri_descriptor_table::allocation dx12_ri_descriptor_table::get_base_allocation()
{
    allocation result {};
    result.valid = true;
    result.index = 0;
    result.cpu_handle = m_allocation.get_cpu_handle(0);
    result.gpu_handle = m_allocation.get_gpu_handle(0);
    return result;
}

bool dx12_ri_descriptor_table::allocation::is_valid() const
{
    return valid;
}

size_t dx12_ri_descriptor_table::allocation::get_table_index() const
{
    return index;
}

}; // namespace ws
