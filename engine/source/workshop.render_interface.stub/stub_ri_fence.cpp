// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_fence.h"

namespace ws {

void stub_ri_fence::wait(size_t value)
{
}

void stub_ri_fence::wait(ri_command_queue& queue, size_t value)
{
}

size_t stub_ri_fence::current_value()
{
    return m_value;
}

void stub_ri_fence::signal(size_t value)
{
    m_value = value;
}

void stub_ri_fence::signal(ri_command_queue& queue, size_t value)
{
    m_value = value;
}

}; // namespace ws
