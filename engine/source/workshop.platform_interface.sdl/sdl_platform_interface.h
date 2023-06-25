// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.platform_interface/platform_interface.h"

#include "workshop.core/utils/event.h"

#include "thirdparty/sdl2/include/SDL.h"

namespace ws {

// ================================================================================================
//  Implementation of platform using the SDL library.
// ================================================================================================
class sdl_platform_interface : public platform_interface
{
public:

    virtual void register_init(init_list& list) override;
    virtual void pump_events() override;

    event<const SDL_Event*> on_sdl_event;

protected:

    result<void> create_sdl(init_list& list);
    result<void> destroy_sdl();

};

}; // namespace ws
