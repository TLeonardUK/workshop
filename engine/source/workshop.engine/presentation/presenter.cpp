// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/presentation/presenter.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/drawing/color.h"
#include "workshop.core/perf/profile.h"

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_world_state.h"

namespace ws {

presenter::~presenter() = default;

presenter::presenter(engine& owner)
    : m_owner(owner)
{
}

void presenter::register_init(init_list& list)
{
    list.add_step(
        "Presentation Resources",
        [this, &list]() -> result<void> { return create_resources(); },
        [this, &list]() -> result<void> { return destroy_resources(); }
    );
}

result<void> presenter::create_resources()
{
    return true;
}

result<void> presenter::destroy_resources()
{
    return true;
}

void presenter::step(const frame_time& time)
{
    profile_marker(profile_colors::system, "presenter");

    std::unique_ptr<render_world_state> state = std::make_unique<render_world_state>();
    state->time = time;

    // TODO: Build the world state.

    m_owner.get_renderer().step(std::move(state));
}

}; // namespace ws
