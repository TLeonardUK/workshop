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
#include "workshop.render_interface.dx12/dx12_ri_raytracing_tlas.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"

namespace ws {

dx12_ri_param_block::dx12_ri_param_block(dx12_render_interface& renderer, dx12_ri_param_block_archetype& archetype)
    : m_renderer(renderer)
    , m_archetype(archetype)
{
    m_fields_set.resize(m_archetype.get_layout_factory().get_field_count());
    m_allocation = m_archetype.allocate();

    m_cpu_shadow_data.resize(m_allocation.size, 0);
}

dx12_ri_param_block::~dx12_ri_param_block()
{
    m_renderer.dequeue_dirty_param_block(this);

    for (auto& [field_index, ref] : m_referenced_textures)
    {
        static_cast<dx12_ri_texture*>(ref.view.texture)->remove_param_block_reference(this);
    }

    if (m_allocation.is_valid())
    {
        m_renderer.defer_delete([allocation = m_allocation, archetype = &m_archetype]()
        {
            archetype->free(allocation);
        });
    }
}

void dx12_ri_param_block::mark_dirty()
{
    std::scoped_lock lock(m_renderer.get_dirty_param_block_mutex());

    if (m_cpu_dirty)
    {
        return;
    }

    m_cpu_dirty = true;
    m_renderer.queue_dirty_param_block(this);
}

void dx12_ri_param_block::upload_state()
{
    std::scoped_lock lock(m_renderer.get_dirty_param_block_mutex());

    m_renderer.get_upload_manager().upload(*m_allocation.buffer, m_cpu_shadow_data, m_allocation.buffer->get_buffer_offset() + m_allocation.offset);
    m_cpu_dirty = false;
}

void* dx12_ri_param_block::consume()
{
    // Warn if not all fields are set.
    for (size_t i = 0; i < m_fields_set.size(); i++)
    {
        if (!m_fields_set[i])
        {
            dx12_ri_layout_factory::field field = m_archetype.get_layout_factory().get_field(i);

            // For buffers, we assume they are intentionally emitted and just clear their references.
            if (field.type == ri_data_type::t_byteaddressbuffer || 
                field.type == ri_data_type::t_rwbyteaddressbuffer)
            {
                clear_buffer(field.name.c_str());
                continue;
            }

            db_warning(renderer, "Consuming param block but field '%s' has not been set and is undefined.", field.name.c_str());
        }
    }

    return m_allocation.address_gpu;
}

ri_param_block_archetype* dx12_ri_param_block::get_archetype()
{
    return &m_archetype;
}

void dx12_ri_param_block::get_table(size_t& index, size_t& offset)
{
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


bool dx12_ri_param_block::set(size_t field_index, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
{
    dx12_ri_layout_factory::field field_info = m_archetype.get_layout_factory().get_field(field_index);

    if (field_info.type != type)
    {
        //db_error(renderer, "Attempting to set field '%s' on param block to incorrect data type '%s' expected '%s'.", field_name, to_string(type).c_str(), to_string(field_info.type).c_str());
        //return;
    }

    if (field_info.size != value_size)
    {
        db_error(renderer, "Value size missmatch for field '%s' on param block. Got '%zi' expected '%zi'.", field_info.name.c_str(), value_size, field_info.size);
        return false;
    }

    db_assert_message(values.size() == value_size, "Array values are not yet supported in param blocks.");

    // Early out if nothing has changed, to avoid the cost of mutating.
    if (m_allocation.is_valid() && m_fields_set[field_info.index])
    {
        void* field_ptr = static_cast<uint8_t*>(m_cpu_shadow_data.data()) + field_info.offset;
        if (memcmp(field_ptr, values.data(), values.size()) == 0)
        {
            return false;
        }
    }

    m_fields_set[field_info.index] = true;

    void* field_ptr = static_cast<uint8_t*>(m_cpu_shadow_data.data()) + field_info.offset;
    memcpy(field_ptr, values.data(), values.size());

    // Matrices are stored column-major but HLSL expects them in row-major, so transpose them.
    transpose_matrices(field_ptr, type);

    mark_dirty();

    return true;
}

bool dx12_ri_param_block::set(const char* field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
{
    dx12_ri_layout_factory::field field_info;
    if (!m_archetype.get_layout_factory().get_field_info(field_name, field_info))
    {
        //db_error(renderer, "Attempting to set unknown field '%s' on param block.", field_name);
        return false;
    }

    return set(field_info.index, values, value_size, type);
}

bool dx12_ri_param_block::set(const char* field_name, const ri_texture& resource)
{ 
    return set(field_name, ri_texture_view(const_cast<ri_texture*>(&resource), ri_texture_view::k_unset, ri_texture_view::k_unset), false);
}

bool dx12_ri_param_block::set(size_t field_index, const ri_texture_view& resource, bool writable, bool do_not_add_references)
{
    const dx12_ri_texture& dx12_resource = static_cast<const dx12_ri_texture&>(*resource.texture);
    uint32_t table_index = static_cast<uint32_t>(dx12_resource.get_main_srv().get_table_index());

    size_t mip = resource.mip;
    size_t slice = resource.slice;

    if (mip == ri_texture_view::k_unset)
    {
        mip = 0;
    }
    if (slice == ri_texture_view::k_unset)
    {
        slice = 0;
    }

    ri_data_type expected_data_type = ri_data_type::t_texture1d;
    switch (dx12_resource.get_dimensions())
    {
    case ri_texture_dimension::texture_1d:
        {
            db_assert(!writable);
            expected_data_type = ri_data_type::t_texture1d;
            break;
        }
    case ri_texture_dimension::texture_2d:
        {
            if (writable)
            {
                expected_data_type = ri_data_type::t_rwtexture2d;
                table_index = static_cast<uint32_t>(dx12_resource.get_uav(slice, mip).get_table_index());
            }
            else
            {
                expected_data_type = ri_data_type::t_texture2d;
            }
            break;
        }
    case ri_texture_dimension::texture_cube:
        {
            if (writable)
            {
                // Cubes write to a specific face so are treated as 2d textures.
                expected_data_type = ri_data_type::t_rwtexture2d;
                table_index = static_cast<uint32_t>(dx12_resource.get_uav(slice, mip).get_table_index());
            }
            else
            {
                // If requesting a specific slice/mip we are treated as a texture2d.
                if (resource.slice != ri_texture_view::k_unset || 
                    resource.mip != ri_texture_view::k_unset)
                {
                    expected_data_type = ri_data_type::t_texture2d;
                    table_index = static_cast<uint32_t>(dx12_resource.get_srv(slice, mip).get_table_index());
                }
                else
                {
                    expected_data_type = ri_data_type::t_texturecube;
                }
            }
            break;
        }
    case ri_texture_dimension::texture_3d:
        {
            db_assert(!writable);
            expected_data_type = ri_data_type::t_texture3d;
            break;
        }
    default:
        {
            db_assert(false);
            break;
        }
    }

    if (set(field_index, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), expected_data_type))
    {
        if (!do_not_add_references)
        {
            add_texture_reference(field_index, resource, writable);
        }
        return true;
    }

    return false;
}

bool dx12_ri_param_block::set(const char* field_name, const ri_texture_view& resource, bool writable)
{
    dx12_ri_layout_factory::field field_info;
    if (!m_archetype.get_layout_factory().get_field_info(field_name, field_info))
    {
        //db_error(renderer, "Attempting to set unknown field '%s' on param block.", field_name);
        return false;
    }

    return set(field_info.index, resource, writable, false);
}

void dx12_ri_param_block::add_texture_reference(size_t field_index, const ri_texture_view& view, bool writable)
{
    std::scoped_lock lock(m_reference_mutex);

    // We only need to store references to partially resident textures, as they are the ones where
    // the view may be arbitrarily changed.
    if (!view.texture->is_partially_resident())
    {
        return;
    }

    // Store reference to texture so we can patch if the views are regenerated (eg. by texture streaming).
    dx12_ri_layout_factory::field field_info = m_archetype.get_layout_factory().get_field(field_index);

    referenced_texture ref;
    ref.view = view;
    ref.writable = writable;

    // Remove old reference (if one exists).
    if (auto iter = m_referenced_textures.find(field_info.index); iter != m_referenced_textures.end())
    {
        static_cast<dx12_ri_texture*>(iter->second.view.texture)->remove_param_block_reference(this);
    }

    // Add new textur reference.
    static_cast<dx12_ri_texture*>(view.texture)->add_param_block_reference(this);

    m_referenced_textures[field_info.index] = ref;
}

void dx12_ri_param_block::clear_texture_references(dx12_ri_texture* texture)
{
    std::scoped_lock lock(m_reference_mutex);

    for (auto iter = m_referenced_textures.begin(); iter != m_referenced_textures.end(); /*empty*/)
    {
        if (iter->second.view.texture == texture)
        {
            dx12_ri_layout_factory::field field_info = m_archetype.get_layout_factory().get_field(iter->first);

            // Clear out the fields that refernce this texture.
            size_t table_index = 0;
            set(iter->first, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), field_info.type);

            iter = m_referenced_textures.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

void dx12_ri_param_block::referenced_texture_modified(dx12_ri_texture* texture)
{
    std::scoped_lock lock(m_reference_mutex);

    for (auto iter = m_referenced_textures.begin(); iter != m_referenced_textures.end(); iter++)
    {
        if (iter->second.view.texture == texture)
        {
            dx12_ri_layout_factory::field field_info = m_archetype.get_layout_factory().get_field(iter->first);

            // Update the reference to point to the new texture srv's/etc
            set(field_info.index, iter->second.view, iter->second.writable, true);
        }
    }
}

bool dx12_ri_param_block::set(const char* field_name, const ri_sampler& resource)
{
    const dx12_ri_sampler& dx12_resource = static_cast<const dx12_ri_sampler&>(resource);
    uint32_t table_index = static_cast<uint32_t>(dx12_resource.get_descriptor_table_index());

    return set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_sampler);
}

bool dx12_ri_param_block::set(const char* field_name, const ri_buffer& resource, bool writable)
{
    const dx12_ri_buffer& dx12_resource = static_cast<const dx12_ri_buffer&>(resource);

    uint32_t table_index;
    ri_data_type type;

    if (writable)
    {
        table_index = static_cast<uint32_t>(dx12_resource.get_uav().get_table_index());
        type = ri_data_type::t_rwbyteaddressbuffer;
    }
    else
    {
        table_index = static_cast<uint32_t>(dx12_resource.get_srv().get_table_index());
        type = ri_data_type::t_byteaddressbuffer;
    }

    return set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), type);
}

bool dx12_ri_param_block::set(const char* field_name, const ri_raytracing_tlas& resource)
{
    const dx12_ri_raytracing_tlas& dx12_resource = static_cast<const dx12_ri_raytracing_tlas&>(resource);
    const dx12_ri_buffer& buffer_resource = static_cast<const dx12_ri_buffer&>(dx12_resource.get_tlas_buffer());

    uint32_t table_index = static_cast<uint32_t>(buffer_resource.get_srv().get_table_index());

    return set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_tlas);
}

bool dx12_ri_param_block::clear_buffer(const char* field_name)
{
    uint32_t table_index = 0;
    return set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_byteaddressbuffer);
}

}; // namespace ws
