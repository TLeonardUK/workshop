// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_command_queue.h"
#include "workshop.renderer/renderer.h"

namespace ws {
    
render_command_queue::render_command_queue(renderer& render, size_t capacity)
    : command_queue(capacity)
    , m_renderer(render)
{
}

// ===========================================================================================
//  Objects
// ===========================================================================================

void render_command_queue::set_object_transform(render_object_id id, const vector3& location, const quat& rotation, const vector3& scale)
{
    struct rc_set_object_transform : public render_command
    {
        render_object_id id;
        vector3 location;
        vector3 scale;
        quat rotation;

        virtual void execute(renderer& renderer) override
        {
            renderer.get_scene_manager().set_object_transform(id, location, rotation, scale);
        }
    };

    rc_set_object_transform cmd;
    cmd.id = id;
    cmd.location = location;
    cmd.scale = scale;
    cmd.rotation = rotation;
    write(cmd);
}

// ================================================================================================
//  Views
// ================================================================================================

render_object_id render_command_queue::create_view(const char* name)
{
    struct rc_create_view : public render_command
    {
        render_object_id id;
        const char* name;

        virtual void execute(renderer& renderer) override
        {
            renderer.get_scene_manager().create_view(id, name);
        }
    };

    rc_create_view cmd;
    cmd.id = m_renderer.next_render_object_id();
    cmd.name = allocate_copy(name);
    write(cmd);
    return cmd.id;
}

void render_command_queue::destroy_view(render_object_id id)
{
    struct rc_destroy_view : public render_command
    {
        render_object_id id;

        virtual void execute(renderer& renderer) override
        {
            renderer.get_scene_manager().destroy_view(id);
        }
    };

    rc_destroy_view cmd;
    cmd.id = id;
    write(cmd);
}

void render_command_queue::set_view_viewport(render_object_id id, const recti& viewport)
{
    struct rc_set_view_viewport : public render_command
    {
        render_object_id id;
        recti viewport;

        virtual void execute(renderer& renderer) override
        {
            renderer.get_scene_manager().set_view_viewport(id, viewport);
        }
    };

    rc_set_view_viewport cmd;
    cmd.id = id;
    cmd.viewport = viewport;
    write(cmd);
}

void render_command_queue::set_view_projection(render_object_id id, float fov, float aspect_ratio, float near_clip, float far_clip)
{
    struct rc_set_view_projection : public render_command
    {
        render_object_id id;
        float fov;
        float aspect_ratio;
        float near_clip;
        float far_clip;

        virtual void execute(renderer& renderer) override
        {
            renderer.get_scene_manager().set_view_projection(id, fov, aspect_ratio, near_clip, far_clip);
        }
    };

    rc_set_view_projection cmd;
    cmd.id = id;
    cmd.fov = fov;
    cmd.aspect_ratio = aspect_ratio;
    cmd.near_clip = near_clip;
    cmd.far_clip = far_clip;
    write(cmd);
}

}; // namespace ws
