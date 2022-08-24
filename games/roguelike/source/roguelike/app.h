// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/app/engine_app.h"

namespace rl {

// ================================================================================================
//  RogueLike core application.
// ================================================================================================
class rl_game_app : public ws::engine_app
{
public:
    virtual std::string get_name() override;

protected:
    virtual ws::result<void> start() override;
    virtual ws::result<void> stop() override;

    virtual void configure_engine(ws::engine& engine) override;

private:

};

};
