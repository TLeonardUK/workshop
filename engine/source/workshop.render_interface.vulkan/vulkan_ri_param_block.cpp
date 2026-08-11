// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_param_block.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_layout_factory.h"
#include "workshop.render_interface.vulkan/vulkan_ri_texture.h"
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_ri_sampler.h"
#include "workshop.render_interface.vulkan/vulkan_ri_raytracing_tlas.h"

#include <cstring>

namespace ws {

vulkan_ri_param_block::vulkan_ri_param_block(vulkan_render_interface& renderer, vulkan_ri_param_block_archetype& archetype)
    : m_renderer(renderer)
    , m_archetype(archetype)
{
    m_fields_set.resize(m_archetype.get_layout_factory().get_field_count());
    m_allocation = m_archetype.allocate();

    m_cpu_shadow_data.resize(m_allocation.size, 0);
}

vulkan_ri_param_block::~vulkan_ri_param_block()
{
    m_renderer.dequeue_dirty_param_block(this);

    for (auto& [field_index, ref] : m_referenced_textures)
    {
        static_cast<vulkan_ri_texture*>(ref.view.texture)->remove_param_block_reference(this);
    }

    if (m_allocation.is_valid())
    {
        m_renderer.defer_delete([allocation = m_allocation, archetype = &m_archetype]()
        {
            archetype->free(allocation);
        });
    }
}

void vulkan_ri_param_block::mark_dirty()
{
    std::scoped_lock lock(m_renderer.get_dirty_param_block_mutex());

    if (m_cpu_dirty)
    {
        return;
    }

    m_cpu_dirty = true;
    m_renderer.queue_dirty_param_block(this);
}

void vulkan_ri_param_block::upload_state()
{
    std::scoped_lock lock(m_renderer.get_dirty_param_block_mutex());

    void* dest = m_allocation.buffer->map(m_allocation.offset, m_cpu_shadow_data.size());
    memcpy(dest, m_cpu_shadow_data.data(), m_cpu_shadow_data.size());
    m_allocation.buffer->unmap(dest);

    m_cpu_dirty = false;
}

VkDeviceAddress vulkan_ri_param_block::consume()
{
    // The gpu address returned here gets baked into a command buffer immediately (as a push
    // constant), but the cpu-side shadow data backing it is otherwise only uploaded later, in
    // a batch, whenever flush_uploads() next runs - which happens on some other command queue
    // execute() call, not necessarily this one. If this block was dirtied too late to have been
    // picked up by an execute() that already ran this frame, the command buffer being recorded
    // right now would reference an address the gpu hasn't actually seen the new contents of yet.
    // Upload synchronously here instead of only relying on the batched path, so the address
    // handed back is always backed by up to date data regardless of that scheduling.
    if (m_cpu_dirty)
    {
        upload_state();
        m_renderer.dequeue_dirty_param_block(this);
    }

    for (size_t i = 0; i < m_fields_set.size(); i++)
    {
        if (!m_fields_set[i])
        {
            vulkan_ri_layout_factory::field field = m_archetype.get_layout_factory().get_field(i);

            if (field.type == ri_data_type::t_byteaddressbuffer ||
                field.type == ri_data_type::t_rwbyteaddressbuffer)
            {
                clear_buffer(field.name_hash);
                continue;
            }

            db_warning(render_interface, "Consuming param block but field '%s' has not been set and is undefined.", field.name_hash.get_string());
        }
    }

    return m_allocation.address_gpu;
}

ri_param_block_archetype* vulkan_ri_param_block::get_archetype()
{
    return &m_archetype;
}

void vulkan_ri_param_block::get_table(size_t& index, size_t& offset)
{
    // Instance/indirect scope blocks are read by shaders straight out of this table entry (no
    // push-constant/consume() step in between), so - same reasoning as consume() above - make
    // sure the cpu-side data has actually reached gpu-visible memory before handing the index
    // out to a caller that may use it in a dispatch recorded immediately afterwards, rather than
    // only relying on flush_uploads() picking it up in time via its own separate scheduling.
    if (m_cpu_dirty)
    {
        upload_state();
        m_renderer.dequeue_dirty_param_block(this);
    }

    m_archetype.get_table(m_allocation, index, offset);
}

bool vulkan_ri_param_block::set(size_t field_index, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
{
    vulkan_ri_layout_factory::field field_info = m_archetype.get_layout_factory().get_field(field_index);

    if (field_info.size != value_size)
    {
        db_error(render_interface, "Value size missmatch for field '%s' on param block. Got '%zi' expected '%zi'.", field_info.name_hash.get_string(), value_size, field_info.size);
        return false;
    }

    db_assert_message(values.size() == value_size, "Array values are not yet supported in param blocks.");

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
    m_archetype.get_layout_factory().transpose_matrices(field_ptr, type);

    mark_dirty();

    return true;
}

bool vulkan_ri_param_block::set(string_hash field_name, const std::span<uint8_t>& values, size_t value_size, ri_data_type type)
{
    vulkan_ri_layout_factory::field field_info;
    if (!m_archetype.get_layout_factory().get_field_info(field_name, field_info))
    {
        return false;
    }

    return set(field_info.index, values, value_size, type);
}

bool vulkan_ri_param_block::set(string_hash field_name, const ri_texture& resource)
{
    return set(field_name, ri_texture_view(const_cast<ri_texture*>(&resource), ri_texture_view::k_unset, ri_texture_view::k_unset), false);
}

bool vulkan_ri_param_block::set(size_t field_index, const ri_texture_view& resource, bool writable, bool do_not_add_references)
{
    vulkan_ri_texture& vk_resource = static_cast<vulkan_ri_texture&>(*resource.texture);
    uint32_t table_index = static_cast<uint32_t>(vk_resource.get_main_srv().index);

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
    switch (vk_resource.get_dimensions())
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
                table_index = static_cast<uint32_t>(vk_resource.get_uav(slice, mip).index);
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
                expected_data_type = ri_data_type::t_rwtexture2d;
                table_index = static_cast<uint32_t>(vk_resource.get_uav(slice, mip).index);
            }
            else
            {
                if (resource.slice != ri_texture_view::k_unset ||
                    resource.mip != ri_texture_view::k_unset)
                {
                    expected_data_type = ri_data_type::t_texture2d;
                    table_index = static_cast<uint32_t>(vk_resource.get_srv(slice, mip).index);
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

bool vulkan_ri_param_block::set(string_hash field_name, const ri_texture_view& resource, bool writable)
{
    vulkan_ri_layout_factory::field field_info;
    if (!m_archetype.get_layout_factory().get_field_info(field_name, field_info))
    {
        return false;
    }

    return set(field_info.index, resource, writable, false);
}

void vulkan_ri_param_block::add_texture_reference(size_t field_index, const ri_texture_view& view, bool writable)
{
    std::scoped_lock lock(m_reference_mutex);

    // We only need to store references to partially resident textures, as they are the ones
    // where the view may be arbitrarily changed.
    if (!view.texture->is_partially_resident())
    {
        return;
    }

    vulkan_ri_layout_factory::field field_info = m_archetype.get_layout_factory().get_field(field_index);

    referenced_texture ref;
    ref.view = view;
    ref.writable = writable;

    if (auto iter = m_referenced_textures.find(field_info.index); iter != m_referenced_textures.end())
    {
        static_cast<vulkan_ri_texture*>(iter->second.view.texture)->remove_param_block_reference(this);
    }

    static_cast<vulkan_ri_texture*>(view.texture)->add_param_block_reference(this);

    m_referenced_textures[field_info.index] = ref;
}

void vulkan_ri_param_block::clear_texture_references(vulkan_ri_texture* texture)
{
    std::scoped_lock lock(m_reference_mutex);

    for (auto iter = m_referenced_textures.begin(); iter != m_referenced_textures.end(); /*empty*/)
    {
        if (iter->second.view.texture == texture)
        {
            vulkan_ri_layout_factory::field field_info = m_archetype.get_layout_factory().get_field(iter->first);

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

void vulkan_ri_param_block::referenced_texture_modified(vulkan_ri_texture* texture)
{
    std::scoped_lock lock(m_reference_mutex);

    for (auto iter = m_referenced_textures.begin(); iter != m_referenced_textures.end(); iter++)
    {
        if (iter->second.view.texture == texture)
        {
            vulkan_ri_layout_factory::field field_info = m_archetype.get_layout_factory().get_field(iter->first);

            set(field_info.index, iter->second.view, iter->second.writable, true);
        }
    }
}

bool vulkan_ri_param_block::set(string_hash field_name, const ri_sampler& resource)
{
    const vulkan_ri_sampler& vk_resource = static_cast<const vulkan_ri_sampler&>(resource);
    uint32_t table_index = static_cast<uint32_t>(vk_resource.get_descriptor_table_index());

    return set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_sampler);
}

bool vulkan_ri_param_block::set(string_hash field_name, const ri_buffer& resource, bool writable)
{
    const vulkan_ri_buffer& vk_resource = static_cast<const vulkan_ri_buffer&>(resource);

    uint32_t table_index;
    ri_data_type type;

    if (writable)
    {
        table_index = static_cast<uint32_t>(vk_resource.get_uav().index);
        type = ri_data_type::t_rwbyteaddressbuffer;
    }
    else
    {
        table_index = static_cast<uint32_t>(vk_resource.get_srv().index);
        type = ri_data_type::t_byteaddressbuffer;
    }

    return set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), type);
}

bool vulkan_ri_param_block::set(string_hash field_name, const ri_raytracing_tlas& resource)
{
    const vulkan_ri_raytracing_tlas& vk_resource = static_cast<const vulkan_ri_raytracing_tlas&>(resource);
    const vulkan_ri_buffer& buffer_resource = static_cast<const vulkan_ri_buffer&>(vk_resource.get_tlas_buffer());

    uint32_t table_index = static_cast<uint32_t>(buffer_resource.get_srv().index);

    return set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_tlas);
}

bool vulkan_ri_param_block::clear_buffer(string_hash field_name)
{
    uint32_t table_index = 0;
    return set(field_name, std::span((uint8_t*)&table_index, sizeof(uint32_t)), sizeof(uint32_t), ri_data_type::t_byteaddressbuffer);
}

}; // namespace ws
