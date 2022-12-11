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
    
    render_view& view = state->views.emplace_back();
    view.viewport = { 0, 0, (int)m_owner.get_main_window().get_width(), (int)m_owner.get_main_window().get_height() };
    view.near_clip = 0.01f;
    view.far_clip = 1000.0f;
    view.field_of_view = 90.0f;
    view.aspect_ratio = (float)view.viewport.width / (float)view.viewport.height;
    view.location = vector3::zero;
    view.rotation = quat::identity;

    // TODO: Build the world state.

    m_owner.get_renderer().step(std::move(state));
}

}; // namespace ws
