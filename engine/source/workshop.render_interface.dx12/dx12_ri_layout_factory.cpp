// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_layout_factory.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"

namespace ws {

dx12_ri_layout_factory::dx12_ri_layout_factory(dx12_render_interface& renderer, ri_data_layout layout)
    : m_renderer(renderer)
    , m_layout(layout)
{
    clear();
    
    // HLSL fields are aligned so they are contained within a vector are 
    // do not straddle boundries.
    static constexpr size_t vector_size = 16;

    size_t offset = 0;
    for (ri_data_layout::field& src_field : layout.fields)
    {
        size_t type_size = ri_bytes_for_data_type(src_field.data_type);

        // Align so we do not straddle vector size.
        size_t remainder = (offset % vector_size);
        if (remainder > 0)
        {
            size_t bytes_left_in_vector = vector_size - remainder;
            if (bytes_left_in_vector < type_size)
            {
                offset += bytes_left_in_vector;
            }
        }

        field dst_field;
        dst_field.name = src_field.name;
        dst_field.type = src_field.data_type;
        dst_field.offset = offset;
        dst_field.size = type_size;
        dst_field.added = false;

        m_fields[dst_field.name] = dst_field;

        offset += dst_field.size;
    }

    m_element_size = offset;

    size_t remainder = (offset % vector_size);
    if (remainder > 0)
    {
        m_element_size += vector_size - remainder;
    }
}

dx12_ri_layout_factory::~dx12_ri_layout_factory()
{
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

void dx12_ri_layout_factory::add(const char* field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
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

        if (type != field.type)
        {
            db_fatal(renderer, "Attempted to add incorrect data type '%s' to layout field '%s' that expected '%s' data type.", to_string(type).c_str(), field.name.c_str(), to_string(field.type).c_str());
        }
        if (value_size != field.size)
        {
            db_fatal(renderer, "Attempted to add data type with incorrect value size '%zi' to layout field '%s'.", value_size, field.name.c_str());
        }

        for (size_t i = 0; i < element_count; i++)
        {
            uint8_t* src = values.data() + (i * value_size);
            uint8_t* dst = m_buffer.data() + (i * m_element_size) + field.offset;
            memcpy(dst, src, value_size);
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
            db_warning(renderer, "Attempting to create buffer, but field '%s' has not been filled. Zeroing out.", field.name.c_str());

            std::vector<uint8_t> data(field.size * m_element_count, 0);
            add(pair.second.name.c_str(), std::span(data.data(), data.size()), field.size, field.type);
        }
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
