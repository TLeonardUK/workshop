// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"

#include <memory>
#include <vector>

namespace ws {

// ================================================================================================
//  Implementation of a command queue using Vulkan.
// ================================================================================================
class vulkan_ri_command_queue : public ri_command_queue
{
public:
    virtual ri_command_list& alloc_command_list() override;
    virtual void execute(ri_command_list& list) override;
    virtual void execute(const std::vector<ri_command_list*>& list) override;
    virtual void begin_event(const color& color, const char* name, ...) override;
    virtual void end_event() override;

private:
    std::vector<std::unique_ptr<vulkan_ri_command_list>> m_command_lists;

};

}; // namespace ws
