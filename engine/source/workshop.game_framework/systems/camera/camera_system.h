// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/ecs/system.h"
#include "workshop.core/math/vector2.h"
#include "workshop.core/math/ray.h"
#include "workshop.core/math/rect.h"

#include "workshop.render_interface/ri_texture.h"

namespace ws {

enum class render_draw_flags;
enum class render_view_flags;
enum class visualization_mode;

// ================================================================================================
//  Responsible for creating and updating render views from all active camera components.
// ================================================================================================
class camera_system : public system
{
public:

    camera_system(object_manager& manager);

    virtual void step(const frame_time& time) override;
 
    virtual void component_removed(object handle, component* comp) override;
    virtual void component_modified(object handle, component* comp, component_modification_source source) override;

public:

    // Public Commands
    
    // Sets the viewport settings for a given camera.
    void set_viewport(object handle, const recti& viewport);

    // Sets the camera to a perspective view with the given settings.
    void set_perspective(object handle, float fov, float aspect_ratio, float min_depth, float max_depth);

    // Sets the camera to an orthographic view with the given settings.
    void set_orthographic(object handle, rect ortho_rect, float min_depth, float max_depth);

    // Sets the draw flags for a given camera.
    void set_draw_flags(object handle, render_draw_flags flags);

    // Sets the view flags for a given camera.
    void set_view_flags(object handle, render_view_flags flags);

    // Sets the render target that the camera should draw to.
    void set_render_target(object handle, ri_texture_view texture);

    // Sets the debug mode this camera renders its view in.
    void set_visualization_mode(object handle, visualization_mode mode);

    // Converts an on screen location to a world space position. Screen location is in 0-1 coordinates.
    vector3 screen_to_world_space(object handle, vector3 screen_space_position);

    // Returns the ray from the cammera, that passes through the given 0-1 coordinates in screen space.
    ray screen_to_ray(object handle, vector2 screen_space_position);

private:
    vector2 m_last_screen_size;

};

}; // namespace ws
