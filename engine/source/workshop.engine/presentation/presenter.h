// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

#include "workshop.render_interface/ri_swapchain.h"

namespace ws {

class engine;
class frame_time;

// ================================================================================================
//  The presenter takes the state of the engine and converts it into rendering commands
//  that can be used to draw the game.
// ================================================================================================
class presenter
{
public:
    presenter(engine& owner);
    ~presenter();

    // Registers all the steps required to initialize the rendering system.
    // Interacting with this class without successfully running these steps is undefined.
    void register_init(init_list& list);

    // Takes the current game state and generates the next render frame.
    void step(const frame_time& time);

protected:
    result<void> create_resources();
    result<void> destroy_resources();

private:
    engine& m_owner;

    std::unique_ptr<ri_swapchain> m_swapchain;

};

}; // namespace ws
