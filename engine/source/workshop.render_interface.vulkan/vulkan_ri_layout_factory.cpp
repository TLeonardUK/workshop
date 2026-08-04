// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_layout_factory.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"

namespace ws {

dx12_ri_layout_factory::dx12_ri_layout_factory(dx12_render_interface& renderer, ri_data_layout layout, ri_layout_usage usage)
    : m_renderer(renderer)
    , m_layout(layout)
    , m_usage(usage)
{
    clear();
    
    // HLSL fields are aligned so they are contained within a vector are 
    // do not straddle boundries.
    static constexpr size_t vector_size = 16;

    size_t offset = 0;
    size_t index = 0;
    for (ri_data_layout::field& src_field : layout.fields)
    {
        size_t type_size = ri_bytes_for_data_type(src_field.data_type);

        // Align so we do not straddle vector size.
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

        // TODO: Handle type alignment rules for non param_block usage.

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

    // Ensure element size is multiple of vector size.
    if (usage == ri_layout_usage::param_block)
    {
        size_t remainder = (offset % vector_size);
        if (remainder > 0)
        {
            m_element_size += vector_size - remainder;
        }
    }
}

dx12_ri_layout_factory::~dx12_ri_layout_factory()
{
}

size_t dx12_ri_layout_factory::get_instance_size()
{
    return m_element_size;
}

void dx12_ri_layout_factory::clear()
{
    m_buffer.clear();
    m_element_size = 0;
    m_element_count = 0;

    for (auto& pair : m_fields)
    {
        pair.second.added = false;
    }
}

void dx12_ri_layout_factory::transpose_matrices(void* field, ri_data_type type)
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

void dx12_ri_layout_factory::add(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
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
        db_fatal(renderer, "Attempted to add inconsistent number of elements. Each add call must contribute the same number of elements.");
    }

    if (auto iter = m_fields.find(field_name); iter == m_fields.end())
    {
        db_fatal(renderer, "Attempted to add data to unknown layout field '%s'.", field_name);
    }
    else
    {
        field& field = iter->second;
        field.added = true;

        if (field.type != ri_data_type::t_compressed_unit_vector)
        {
            if (type != field.type)
            {
                db_fatal(renderer, "Attempted to add incorrect data type '%s' to layout field '%s' that expected '%s' data type.", to_string(type).c_str(), field.name_hash.get_string(), to_string(field.type).c_str());
            }
            if (value_size != field.size)
            {
                db_fatal(renderer, "Attempted to add data type with incorrect value size '%zi' to layout field '%s'.", value_size, field.name_hash.get_string());
            }
        }

        for (size_t i = 0; i < element_count; i++)
        {
            uint8_t* src = values.data() + (i * value_size);
            uint8_t* dst = m_buffer.data() + (i * m_element_size) + field.offset;

            // Compress if required.
            // TODO: Figure out how to move all of these to happen during asset compilation.
            if (field.type == ri_data_type::t_compressed_unit_vector)
            {
                if (type != ri_data_type::t_float3)
                {
                    db_fatal(renderer, "Attempted to add compressed unit vector to layout field '%s' with invalid source data type.", field.name_hash.get_string());
                }

                vector3* vec = reinterpret_cast<vector3*>(src);
                *reinterpret_cast<float*>(dst) = compress_unit_vector(*vec);
            }
            // We can just direct copy the geometry data.
            else
            {
                memcpy(dst, src, value_size);
            }

            // We store our matrices as column-major but directx expects them to be in row-major, so transpose them.
            transpose_matrices(dst, type);
        }
    }
}

void dx12_ri_layout_factory::validate()
{
    for (auto& pair : m_fields)
    {
        field& field = pair.second;

        if (!field.added)
        {
            db_warning(renderer, "Attempting to create buffer, but field '%s' has not been filled. Zeroing out.", field.name_hash.get_string());

            std::vector<uint8_t> data(field.size * m_element_count, 0);
            add(pair.second.name_hash, std::span(data.data(), data.size()), field.size, field.type);
        }
    }
}

size_t dx12_ri_layout_factory::get_field_count()
{
    return m_fields.size();
}

dx12_ri_layout_factory::field dx12_ri_layout_factory::get_field(size_t index)
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

bool dx12_ri_layout_factory::get_field_info(string_hash name, field& info)
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

std::unique_ptr<ri_buffer> dx12_ri_layout_factory::create_vertex_buffer(const char* name)
{
    validate();

    ri_buffer::create_params params;
    params.element_count = m_element_count;
    params.element_size = m_element_size;
    params.usage = ri_buffer_usage::vertex_buffer;
    params.linear_data = std::span { m_buffer.data(), m_buffer.size() };    
    return m_renderer.create_buffer(params, name);
}

#if 0
std::unique_ptr<ri_buffer> dx12_ri_layout_factory::create_index_buffer(const char* name, const std::vector<uint16_t>& indices)
{
    validate();

    if (m_element_count > std::numeric_limits<uint16_t>::max())
    {
        db_fatal(renderer, "Attempted to create index buffer with 16 bit indices, but there are more vertices than can fit within numeric limits.");
    }

    for (uint16_t index : indices)
    {
        if (index >= m_element_count)
        {
            db_fatal(renderer, "Attempted to create index buffer with indices beyond bounds of available vertices.");
        }
    }

    ri_buffer::create_params params;
    params.element_count = indices.size();
    params.element_size = sizeof(uint16_t);
    params.usage = ri_buffer_usage::index_buffer;
    params.linear_data = std::span { (uint8_t*)indices.data(), indices.size() * sizeof(uint16_t) };
    return m_renderer.create_buffer(params, name);
}
#endif

std::unique_ptr<ri_buffer> dx12_ri_layout_factory::create_index_buffer(const char* name, const std::vector<uint32_t>& indices)
{
    validate();

    if (m_element_count > std::numeric_limits<uint32_t>::max())
    {
        db_fatal(renderer, "Attempted to create index buffer with 32 bit indices, but there are more vertices than can fit within numeric limits.");
    }

    for (uint16_t index : indices)
    {
        if (index >= m_element_count)
        {
            db_fatal(renderer, "Attempted to create index buffer with indices beyond bounds of available vertices.");
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
