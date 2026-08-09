// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.stub/stub_ri_command_queue.h"

namespace ws {

ri_command_list& stub_ri_command_queue::alloc_command_list()
{
    m_command_lists.push_back(std::make_unique<stub_ri_command_list>());
    return *m_command_lists.back();
}

void stub_ri_command_queue::execute(ri_command_list& list)
{
}

void stub_ri_command_queue::execute(const std::vector<ri_command_list*>& list)
{
}

void stub_ri_command_queue::begin_event(const color& color, const char* name, ...)
{
}

void stub_ri_command_queue::end_event()
{
}

}; // namespace ws
