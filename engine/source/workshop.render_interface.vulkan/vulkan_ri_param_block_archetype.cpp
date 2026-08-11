// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_param_block_archetype.h"
#include "workshop.render_interface.vulkan/vulkan_ri_param_block.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_layout_factory.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

vulkan_ri_param_block_archetype::vulkan_ri_param_block_archetype(vulkan_render_interface& renderer, const ri_param_block_archetype::create_params& params, const char* debug_name)
    : m_renderer(renderer)
    , m_create_params(params)
    , m_debug_name(debug_name)
{
    bool indirect_referenced = (m_create_params.scope == ri_data_scope::instance || m_create_params.scope == ri_data_scope::indirect);

    m_layout_factory = m_renderer.create_layout_factory(
        m_create_params.layout,
        indirect_referenced ? ri_layout_usage::buffer : ri_layout_usage::param_block
    );
    m_instance_size = m_layout_factory->get_instance_size();

    // Instance param blocks are read as byte address buffers not cbuffers so don't need to
    // follow the cbuffer alignment rules.
    if (indirect_referenced)
    {
        m_instance_stride = m_instance_size;
    }
    else
    {
        m_instance_stride = math::round_up_multiple(m_instance_size, k_instance_alignment);
    }
}

vulkan_ri_param_block_archetype::~vulkan_ri_param_block_archetype()
{
    for (size_t i = 0; i < m_pages.size(); i++)
    {
        alloc_page& instance = m_pages[i];
        if (instance.free_list.size() != k_page_size)
        {
            db_warning(render_interface, "Param block archetype '%s' is being destroyed but not all param blocks have been deallocated.", m_debug_name.c_str());
        }

        bool indirect_referenced = (m_create_params.scope == ri_data_scope::instance || m_create_params.scope == ri_data_scope::indirect);
        if (indirect_referenced)
        {
            m_renderer.defer_delete([renderer = &m_renderer, srv = instance.srv]()
            {
                if (srv.is_valid)
                {
                    renderer->get_descriptor_table(ri_descriptor_table::buffer).free(srv);
                }
            });
        }

        instance.buffer = nullptr;
    }

    m_pages.clear();
}

result<void> vulkan_ri_param_block_archetype::create_resources()
{
    add_page();
    return true;
}

vulkan_ri_param_block_archetype::allocation vulkan_ri_param_block_archetype::allocate()
{
    std::scoped_lock lock(m_allocation_mutex);

    while (true)
    {
        for (size_t i = 0; i < m_pages.size(); i++)
        {
            alloc_page& instance = m_pages[i];
            if (instance.free_list.size() > 0)
            {
                uint16_t index = instance.free_list.back();
                instance.free_list.pop_back();

                allocation result;
                result.offset = (index * m_instance_stride);
                result.address_gpu = instance.base_address_gpu + result.offset;
                result.buffer = static_cast<vulkan_ri_buffer*>(instance.buffer.get());
                result.pool_index = i;
                result.allocation_index = index;
                result.size = m_instance_stride;
                result.valid = true;

                return result;
            }
        }

        add_page();
    }

    return {};
}

void vulkan_ri_param_block_archetype::get_table(allocation alloc, size_t& index, size_t& offset)
{
    std::scoped_lock lock(m_allocation_mutex);

    index = m_pages[alloc.pool_index].srv.index;
    offset = (alloc.allocation_index * m_instance_stride);
}

void vulkan_ri_param_block_archetype::free(allocation alloc)
{
    std::scoped_lock lock(m_allocation_mutex);

    m_pages[alloc.pool_index].free_list.push_back(alloc.allocation_index);
}

void vulkan_ri_param_block_archetype::add_page()
{
    memory_scope mem_scope(memory_type::rendering__vram__param_blocks, string_hash(m_debug_name));

    std::scoped_lock lock(m_allocation_mutex);

    alloc_page& instance = m_pages.emplace_back();

    std::string debug_name = string_format("Param Block Page [%s]", m_debug_name.c_str());

    ri_buffer::create_params params;
    params.element_count = k_page_size;
    params.element_size = m_instance_stride;
    params.usage = ri_buffer_usage::param_block;
    instance.buffer = m_renderer.create_buffer(params, debug_name.c_str());

    vulkan_ri_buffer* vulkan_buffer = static_cast<vulkan_ri_buffer*>(instance.buffer.get());

    instance.base_address_gpu = vulkan_buffer->get_gpu_address();

    instance.free_list.resize(k_page_size);
    for (size_t i = 0; i < k_page_size; i++)
    {
        instance.free_list[i] = (uint16_t)((k_page_size - 1) - i);
    }

    // If using this param block as instance data, we need to create a bindless raw/byte-
    // address buffer entry so we can access it by index.
    if (m_create_params.scope == ri_data_scope::instance || m_create_params.scope == ri_data_scope::indirect)
    {
        instance.srv = m_renderer.get_descriptor_table(ri_descriptor_table::buffer).allocate();
        m_renderer.get_descriptor_table(ri_descriptor_table::buffer).write_storage_buffer(
            instance.srv,
            vulkan_buffer->get_buffer(),
            vulkan_buffer->get_buffer_offset(),
            k_page_size * m_instance_stride);
    }
}

std::unique_ptr<ri_param_block> vulkan_ri_param_block_archetype::create_param_block()
{
    return std::make_unique<vulkan_ri_param_block>(m_renderer, *this);
}

vulkan_ri_layout_factory& vulkan_ri_param_block_archetype::get_layout_factory()
{
    return static_cast<vulkan_ri_layout_factory&>(*m_layout_factory);
}

const char* vulkan_ri_param_block_archetype::get_name()
{
    return m_debug_name.c_str();
}

const vulkan_ri_param_block_archetype::create_params& vulkan_ri_param_block_archetype::get_create_params()
{
    return m_create_params;
}

size_t vulkan_ri_param_block_archetype::get_size()
{
    return m_layout_factory->get_instance_size();
}

bool vulkan_ri_param_block_archetype::allocation::is_valid() const
{
    return valid;
}

}; // namespace ws
