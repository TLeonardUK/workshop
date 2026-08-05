// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_fence.h"

namespace ws {

// ================================================================================================
//  Implementation of a fence using Vulkan.
// ================================================================================================
class vulkan_ri_fence : public ri_fence
{
public:
    virtual void wait(size_t value) override;
    virtual void wait(ri_command_queue& queue, size_t value) override;
    virtual size_t current_value() override;
    virtual void signal(size_t value) override;
    virtual void signal(ri_command_queue& queue, size_t value) override;

private:
    size_t m_value = 0;

};

}; // namespace ws
