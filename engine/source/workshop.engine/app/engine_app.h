// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/app/app.h"

namespace ws {

class engine;

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

    // Called just before the engine is initialized, can be used to configure behaviour
    // of the engine, such as its renderer/etc, before its initialized.
    virtual void configure_engine(engine& engine);
    
private:

    std::unique_ptr<engine> m_engine;

};

}; // namespace ws
