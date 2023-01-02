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
    auto& cmd_queue = m_owner.get_renderer().get_command_queue();

    render_object_id view_id = cmd_queue.create_view("Main View");
    cmd_queue.set_view_viewport(view_id, recti(0, 0, m_owner.get_main_window().get_width(), m_owner.get_main_window().get_height()));
    cmd_queue.set_view_projection(view_id, 90.0f, 1.33f, 0.1f, 10000.0f);
    cmd_queue.set_object_transform(view_id, vector3::zero, quat::identity, vector3::one);

//    render_object_id object_id = cmd_queue.create_static_mesh("Main Object");
//    cmd_queue.set_static_mesh(object_id, mesh);
//    cmd_queue.set_static_mesh_material(object_id, material);
//    cmd_queue.set_object_transform(object_id, vector3(0.0f, 0.0f, 10.0f), quat::identity, vector3::one);

//    cmd_queue.set_object_attachment(view_id, other);


    //cmd_queue.draw_debug_sphere();

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
    
    m_owner.get_renderer().step(std::move(state));
}

}; // namespace ws
