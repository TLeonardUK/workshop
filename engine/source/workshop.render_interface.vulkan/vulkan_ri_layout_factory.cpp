// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_layout_factory.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"

#include <cstring>

namespace ws {

vulkan_ri_layout_factory::vulkan_ri_layout_factory(vulkan_render_interface& renderer, ri_data_layout layout, ri_layout_usage usage)
    : m_renderer(renderer)
    , m_layout(layout)
    , m_usage(usage)
{
    clear();

    // HLSL fields are aligned so they are contained within a vector and do not straddle
    // boundaries.
    static constexpr size_t vector_size = 16;

    size_t offset = 0;
    size_t index = 0;
    for (ri_data_layout::field& src_field : layout.fields)
    {
        size_t type_size = ri_bytes_for_data_type(src_field.data_type);

        if (usage == ri_layout_usage::param_block)
        {
            size_t remainder = (offset % vector_size);
            if (remainder > 0)
            {
                size_t bytes_left_in_vector = vector_size - remainder;
                if (bytes_left_in_vector < type_size)
                {
                    offset += bytes_left_in_vector;
                }
            }
        }

        field dst_field;
        dst_field.name_hash = string_hash(src_field.name);
        dst_field.type = src_field.data_type;
        dst_field.offset = offset;
        dst_field.size = type_size;
        dst_field.added = false;
        dst_field.index = index++;

        m_fields[dst_field.name_hash] = dst_field;

        offset += dst_field.size;
    }

    m_element_size = offset;

    if (usage == ri_layout_usage::param_block)
    {
        size_t remainder = (offset % vector_size);
        if (remainder > 0)
        {
            m_element_size += vector_size - remainder;
        }
    }
}

vulkan_ri_layout_factory::~vulkan_ri_layout_factory()
{
}

size_t vulkan_ri_layout_factory::get_instance_size()
{
    return m_element_size;
}

void vulkan_ri_layout_factory::clear()
{
    m_buffer.clear();
    m_element_size = 0;
    m_element_count = 0;

    for (auto& pair : m_fields)
    {
        pair.second.added = false;
    }
}

void vulkan_ri_layout_factory::transpose_matrices(void* field, ri_data_type type)
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

void vulkan_ri_layout_factory::add(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
{
    db_assert(!values.empty());

    size_t element_count = values.size() / value_size;

    if (m_element_count == 0)
    {
        m_element_count = element_count;
        m_buffer.resize(m_element_count * m_element_size);
    }
    else if (element_count != m_element_count)
    {
        db_fatal(render_interface, "Attempted to add inconsistent number of elements. Each add call must contribute the same number of elements.");
    }

    if (auto iter = m_fields.find(field_name); iter == m_fields.end())
    {
        db_fatal(render_interface, "Attempted to add data to unknown layout field '%s'.", field_name.get_string());
    }
    else
    {
        field& field = iter->second;
        field.added = true;

        if (field.type != ri_data_type::t_compressed_unit_vector)
        {
            if (type != field.type)
            {
                db_fatal(render_interface, "Attempted to add incorrect data type '%s' to layout field '%s' that expected '%s' data type.", to_string(type).c_str(), field.name_hash.get_string(), to_string(field.type).c_str());
            }
            if (value_size != field.size)
            {
                db_fatal(render_interface, "Attempted to add data type with incorrect value size '%zi' to layout field '%s'.", value_size, field.name_hash.get_string());
            }
        }

        for (size_t i = 0; i < element_count; i++)
        {
            uint8_t* src = values.data() + (i * value_size);
            uint8_t* dst = m_buffer.data() + (i * m_element_size) + field.offset;

            if (field.type == ri_data_type::t_compressed_unit_vector)
            {
                if (type != ri_data_type::t_float3)
                {
                    db_fatal(render_interface, "Attempted to add compressed unit vector to layout field '%s' with invalid source data type.", field.name_hash.get_string());
                }

                vector3* vec = reinterpret_cast<vector3*>(src);
                *reinterpret_cast<float*>(dst) = compress_unit_vector(*vec);
            }
            else
            {
                memcpy(dst, src, value_size);
            }

            transpose_matrices(dst, type);
        }
    }
}

void vulkan_ri_layout_factory::validate()
{
    for (auto& pair : m_fields)
    {
        field& field = pair.second;

        if (!field.added)
        {
            db_warning(render_interface, "Attempting to create buffer, but field '%s' has not been filled. Zeroing out.", field.name_hash.get_string());

            std::vector<uint8_t> data(field.size * m_element_count, 0);
            add(pair.second.name_hash, std::span(data.data(), data.size()), field.size, field.type);
        }
    }
}

size_t vulkan_ri_layout_factory::get_field_count()
{
    return m_fields.size();
}

vulkan_ri_layout_factory::field vulkan_ri_layout_factory::get_field(size_t index)
{
    for (auto& pair : m_fields)
    {
        if (pair.second.index == index)
        {
            return pair.second;
        }
    }

    db_assert_message(false, "Index out of range.");
    return {};
}

bool vulkan_ri_layout_factory::get_field_info(string_hash name, field& info)
{
    if (auto iter = m_fields.find(name); iter != m_fields.end())
    {
        info = iter->second;
        return true;
    }
    else
    {
        return false;
    }
}

std::unique_ptr<ri_buffer> vulkan_ri_layout_factory::create_vertex_buffer(const char* name)
{
    validate();

    ri_buffer::create_params params;
    params.element_count = m_element_count;
    params.element_size = m_element_size;
    params.usage = ri_buffer_usage::vertex_buffer;
    params.linear_data = std::span{ m_buffer.data(), m_buffer.size() };
    return m_renderer.create_buffer(params, name);
}

std::unique_ptr<ri_buffer> vulkan_ri_layout_factory::create_index_buffer(const char* name, const std::vector<uint32_t>& indices)
{
    validate();

    if (m_element_count > std::numeric_limits<uint32_t>::max())
    {
        db_fatal(render_interface, "Attempted to create index buffer with 32 bit indices, but there are more vertices than can fit within numeric limits.");
    }

    for (uint32_t index : indices)
    {
        if (index >= m_element_count)
        {
            db_fatal(render_interface, "Attempted to create index buffer with indices beyond bounds of available vertices.");
        }
    }

    ri_buffer::create_params params;
    params.element_count = indices.size();
    params.element_size = sizeof(uint32_t);
    params.usage = ri_buffer_usage::index_buffer;
    params.linear_data = std::span{ (uint8_t*)indices.data(), indices.size() * sizeof(uint32_t) };
    return m_renderer.create_buffer(params, name);
}

}; // namespace ws
