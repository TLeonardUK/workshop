// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_buffer.h"

namespace ws {

stub_ri_buffer::stub_ri_buffer(const create_params& params, const char* debug_name)
    : m_params(params)
    , m_debug_name(debug_name ? debug_name : "")
    , m_backing_store(params.element_count * params.element_size)
{
}

size_t stub_ri_buffer::get_element_count()
{
    return m_params.element_count;
}

size_t stub_ri_buffer::get_element_size()
{
    return m_params.element_size;
}

const char* stub_ri_buffer::get_debug_name()
{
    return m_debug_name.c_str();
}

ri_resource_state stub_ri_buffer::get_initial_state()
{
    return ri_resource_state::initial;
}

void* stub_ri_buffer::map(size_t offset, size_t size)
{
    if (m_backing_store.empty())
    {
        return nullptr;
    }
    return m_backing_store.data() + offset;
}

void stub_ri_buffer::unmap(void* pointer)
{
}

}; // namespace ws
