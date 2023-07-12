// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/command_queue.h"
#include "workshop.assets/asset_manager.h"

#include "workshop.core/math/rect.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/math/aabb.h"
#include "workshop.core/math/sphere.h"
#include "workshop.core/math/frustum.h"
#include "workshop.core/math/cylinder.h"
#include "workshop.core/math/hemisphere.h"
#include "workshop.core/drawing/color.h"

namespace ws {

class renderer;
class model;
enum class visualization_mode;

// Used as an opaque reference to objects created through the use of the render command queue.
using render_object_id = size_t;

// ================================================================================================
//  The render command queue is used by engine code to queue commands that modify
//  the state of the world being rendered.
// 
//  This provides a direct seperation between all rendering related code and all engine/game level
//  code. The render world state should only ever be modified via this interface. Handling
//  frame buffering/pipeline is not required when going through this interface.
// 
//  The API for this is designed to be simple and expose no render classes directly. Instead 
//  commands return opaque render_id types that can be used in future to reference objects
//  created via the commands.
// ================================================================================================
class render_command_queue : public command_queue
{
public:
    render_command_queue(renderer& render, size_t capacity);

public:

    // ===========================================================================================
    //  Global
    // ===========================================================================================

    // Sets the debug mode we should use for rendering the output.
    void set_visualization_mode(visualization_mode mode);

    // ===========================================================================================
    //  Debug rendering.
    // ===========================================================================================

    // Draws various debug wireframe primitives.
    void draw_line(const vector3& start, const vector3& end, const color& color);
    void draw_aabb(const aabb& bounds, const color& color);
    void draw_obb(const obb& bounds, const color& color);
    void draw_sphere(const sphere& bounds, const color& color);
    void draw_frustum(const frustum& bounds, const color& color);
    void draw_triangle(const vector3& a, const vector3& b, const vector3& c, const color& color);
    void draw_cylinder(const cylinder& bounds, const color& color);
    void draw_capsule(const cylinder& bounds, const color& color);
    void draw_hemisphere(const hemisphere& bounds, const color& color, bool horizontal_bands = true);
    void draw_cone(const vector3& origin, const vector3& end, float radius, const color& color);
    void draw_arrow(const vector3& start, const vector3& end, const color& color);
    void draw_truncated_cone(const vector3& start, const vector3& end, float start_radius, float end_radius, const color& color);

    // ===========================================================================================
    //  Objects
    // ===========================================================================================

    // Sets the location and rotation of a render objects.
    void set_object_transform(render_object_id id, const vector3& location, const quat& rotation, const vector3& scale);

    // ===========================================================================================
    //  Views
    // ===========================================================================================

    // Creates a new view of the scene that will be rendered to the backbuffer. 
    render_object_id create_view(const char* name);

    // Removes a view previously created with create_view.
    void destroy_view(render_object_id id);

    // Set the viewport in pixel space to which to render the view.    
    void set_view_viewport(render_object_id id, const recti& viewport);

    // Sets the projection parameters of the view.
    void set_view_projection(render_object_id id, float fov, float aspect_ratio, float near_clip, float far_clip);

    // ===========================================================================================
    //  Static meshes
    // ===========================================================================================

    // Creates a non-animated static mesh that will be rendered in the scene.
    render_object_id create_static_mesh(const char* name);

    // Destroys a static mesh previously created with create_static_mesh.
    void destroy_static_mesh(render_object_id id);

    // Set the model that will be rendered on the static mesh.
    void set_static_mesh_model(render_object_id id, const asset_ptr<model>& model);

private:
    renderer& m_renderer;

};

}; // namespace ws
