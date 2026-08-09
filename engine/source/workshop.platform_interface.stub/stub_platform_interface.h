// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.platform_interface/platform_interface.h"

namespace ws {

// ================================================================================================
//  Stub implementation of platform_interface, performs no actual work. Useful for headless
//  builds or platforms that do not have a real platform implementation available.
// ================================================================================================
class stub_platform_interface : public platform_interface
{
public:

    virtual void register_init(init_list& list) override;
    virtual void pump_events() override;

};

}; // namespace ws
