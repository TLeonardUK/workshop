// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_param_block.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_upload_manager.h"
#include "workshop.render_interface.dx12/dx12_ri_layout_factory.h"
#include "workshop.render_interface.dx12/dx12_ri_texture.h"
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_ri_sampler.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"

namespace ws {

dx12_ri_param_block::dx12_ri_param_block(dx12_render_interface& renderer, dx12_ri_param_block_archetype& archetype)
    : m_renderer(renderer)
    , m_archetype(archetype)
{
    m_fields_set.resize(m_archetype.get_layout_factory().get_field_count());
}

dx12_ri_param_block::~dx12_ri_param_block()
{
    if (m_allocation.is_valid())
    {
        m_renderer.defer_delete([allocation = m_allocation, archetype = &m_archetype]()
        {
            archetype->free(allocation);
        });
    }
}

void dx12_ri_param_block::mutate()
{
    if (m_use_count == m_last_mutate_use_count)
    {
        return;
    }

    m_last_mutate_use_count = m_use_count;

    // Free up existing allocation.
    dx12_ri_param_block_archetype::allocation old_allocation = m_allocation;

    // Allocate new param block.
    m_allocation = m_archetype.allocate();

    if (old_allocation.is_valid())
    {
        // Copy existing data over to new param block.
        memcpy(m_allocation.cpu_address, old_allocation.cpu_address, old_allocation.size);

        // Free old allocation.
        m_renderer.defer_delete([allocation = old_allocation, archetype = &m_archetype]()
        {
            archetype->free(allocation);
        });
    }
}

void* dx12_ri_param_block::consume()
{
    m_use_count++;

    // If no param block has been allocated yet, allocate one now.
    if (!m_allocation.is_valid())
    {
        mutate();
    }

    // Warn if not all fields are set.
    for (size_t i = 0; i < m_fields_set.size(); i++)
    {
        if (!m_fields_set[i])
        {
            dx12_ri_layout_factory::field field = m_archetype.get_layout_factory().get_field(i);
            db_warning(renderer, "Consuming param block but field '%s' has not been set and is undefined.", field.name.c_str());
        }
    }

    return m_allocation.gpu_address;
}

ri_param_block_archetype* dx12_ri_param_block::get_archetype()
{
    return &m_archetype;
}

void dx12_ri_param_block::get_table(size_t& index, size_t& offset)
{
    // If no param block has been allocated yet, we need to allocate one to return the table information.
    if (!m_allocation.is_valid())
    {
        mutate();
    }

    // We need to consume the param block if we are referencing the table indices. Otherwise indirect references
    // to param blocks will never increase the use count and never mutate.
    consume();

    m_archetype.get_table(m_allocation, index, offset);
}

void dx12_ri_param_block::transpose_matrices(void* field, ri_data_type type)
{
    switch (type)
    {
    case ri_data_type::t_double2x2:     *reinterpret_cast<matrix2d*>(field) = reinterpret_cast<matrix2d*>(field)->transpose(); break;
    case ri_data_type::t_double3x3:     *reinterpret_cast<matrix3d*>(field) = reinterpret_cast<matrix3d*>(field)->transpose(); break;
    case ri_data_type::t_double4x4:     *reinterpret_cast<matrix4d*>(field) = reinterpret_cast<matrix4d*>(field)->transpose(); break;
    case ri_data_type::t_float2x2:      *reinterpret_cast<matrix2*>(field) = reinterpret_cast<matrix2*>(field)->transpose(); break;
    case ri_data_type::t_float3x3:      *reinterpret_cast<matrix3*>(field) = reinterpret_cast<matrix3*>(field)->transpose(); break;
    case ri_data_type::t_float4x4:      *reinterpret_cast<matrix4*>(field) = reinterpret_cast<matrix4*>(field)->transpose(); break;
    }
}

void dx12_ri_param_block::set(const char* field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
{
    dx12_ri_layout_factory::field field_info;
    if (!m_archetype.get_layout_factory().get_field_info(field_name, field_info))
    {
        db_error(renderer, "Attempting to set unknown field '%s' on param block.", field_name);
        return;
    }

    if (field_info.type != type)
    {
        //db_error(renderer, "Attempting to set field '%s' on param block to incorrect data type '%s' expected '%s'.", field_name, to_string(type).c_str(), to_string(field_info.type).c_str());
        //return;
    }

    if (field_info.size != value_size)
    {
        db_error(renderer, "Value size missmatch for field '%s' on param block. Got '%zi' expected '%zi'.", field_name, value_size, field_info.size);
        return;
    }

    db_assert_message(values.size() == value_size, "Array values are not yet supported in param blocks.");

    // Early out if nothing has changed, to avoid the cost of mutating.
    if (m_allocation.is_valid() && m_fields_set[field_info.index])
    {
        void* field_ptr = static_cast<uint8_t*>(m_allocation.cpu_address) + field_info.offset;
        if (memcmp(field_ptr, values.data(), values.size()) == 0)
        {
            return;
        }
    }

    mutate();

    m_fields_set[field_info.index] = true;

    void* field_ptr = static_cast<uint8_t*>(m_allocation.cpu_address) + field_info.offset;
    memcpy(field_ptr, values.data(), values.size());

    // Matrices are stored column-major but HLSL expects them in row-major, so transpose them.
    transpose_matrices(field_ptr, type);
}

void dx12_ri_param_block::set(const char* field_name, const ri_texture& resource)
{    
    const dx12_ri_texture& dx12_resource = static_cast<const dx12_ri_texture&>(resource);
    uint32_t table_index = static_cast<uint32_t>(dx12_resource.get_srv().get_table_index());

    ri_data_type expected_data_type = ri_data_type::t_texture1d;
    switch (dx12_resource.get_dimensions())
    {
    case ri_texture_dimension::texture_1d:
        {
            expected_data_type = ri_data_type::t_texture1d;
            break;
        }
    case ri_texture_dimension::texture_2d:
        {
            expected_data_type = ri_data_type::t_texture2d;
            break;
        }
    case ri_texture_dimension::texture_3d:
        {
            expected_data_type = ri_data_type::t_texture3d;
            break;
        }
    case ri_texture_dimension::texture_cube:
        {
            expected_data_type = ri_data_type::t_texturecube;
            break;
        }
    default:
        {
            db_assert(false);
            break;
        }
    }

    set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), expected_data_type);
}

void dx12_ri_param_block::set(const char* field_name, const ri_sampler& resource)
{
    const dx12_ri_sampler& dx12_resource = static_cast<const dx12_ri_sampler&>(resource);
    uint32_t table_index = static_cast<uint32_t>(dx12_resource.get_descriptor_table_index());

    set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_sampler);
}

void dx12_ri_param_block::set(const char* field_name, const ri_buffer& resource)
{
    const dx12_ri_buffer& dx12_resource = static_cast<const dx12_ri_buffer&>(resource);
    uint32_t table_index = static_cast<uint32_t>(dx12_resource.get_srv().get_table_index());

    set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_byteaddressbuffer);
}

void dx12_ri_param_block::clear_buffer(const char* field_name)
{
    uint32_t table_index = 0;
    set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_byteaddressbuffer);
}

}; // namespace ws
