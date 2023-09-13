// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/app/engine_app.h"

namespace ws {

// ================================================================================================
//  Eample core application.
// ================================================================================================
class example_game_app : public engine_app
{
public:
    virtual std::string get_name() override;

protected:
    virtual ws::result<void> start() override;
    virtual ws::result<void> stop() override;

    virtual void configure_engine(engine& engine) override;

    virtual void step(const frame_time& time) override;

};

};
