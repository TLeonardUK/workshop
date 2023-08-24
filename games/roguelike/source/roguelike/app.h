// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/app/engine_app.h"
#include "workshop.engine/ecs/object.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/utils/event.h"
#include "workshop.renderer/render_command_queue.h"

namespace ws {

// ================================================================================================
//  RogueLike core application.
// ================================================================================================
class rl_game_app : public engine_app
{
public:
    virtual std::string get_name() override;

protected:
    virtual ws::result<void> start() override;
    virtual ws::result<void> stop() override;

    virtual void configure_engine(engine& engine) override;

    void step(const frame_time& time);

private:
    event<frame_time>::delegate_ptr m_on_step_delegate;

    render_object_id m_view_id;
    render_object_id m_light_id;
    render_object_id m_light_probe_id;

    vector3 m_view_position = vector3::zero;
    vector3 m_view_rotation_euler = vector3::zero;
    quat m_view_rotation = quat::identity;

    int m_mouse_control_frames = 0;

    std::vector<render_object_id> m_rotating_objects;

    object m_camera_object;

    struct moving_light
    {
        render_object_id id;
        float distance;
        float angle;
        float speed;
        float range;
        float height;
    };
    std::vector<moving_light> m_moving_lights;
};

};
