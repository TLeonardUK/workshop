// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/app/app.h"
#include "workshop.core/utils/event.h"
#include "workshop.core/utils/frame_time.h"

namespace ws {

class engine;
class object_manager;

// ================================================================================================
//  Basic app that hosts an instance of the game engine. All game-style applications should derive
//  from here. Commandlet style applications should derive from the base app.
// ================================================================================================
class engine_app : public app
{
public:

    engine_app();
    virtual ~engine_app();

    // Gets the engine instance hosted by this app.
    engine& get_engine();

protected:

    virtual void register_init(init_list& list) override;
    virtual result<void> loop() override;

    // Calleded every time the engine steps the world forwards.
    virtual void step(const frame_time& time);

    // Called just before the engine is initialized, can be used to configure behaviour
    // of the engine, such as its renderer/etc, before its initialized.
    virtual void configure_engine(engine& engine);
    
private:

    std::unique_ptr<engine> m_engine;

    event<frame_time>::delegate_ptr m_on_step_delegate;

};

}; // namespace ws
