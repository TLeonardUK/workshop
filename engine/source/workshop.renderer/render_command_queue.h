// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/command_queue.h"

#include "workshop.core/math/rect.h"
#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"

namespace ws {

class renderer;

// Used as an opaque reference to objects created through the use of the render command queue.
using render_object_id = size_t;

// ================================================================================================
//  Base struct for all derived render commands. 
// ================================================================================================
struct render_command : public command
{
    // Called on the render thread when the command queue 
    // is being processed.
    virtual void execute(renderer& renderer) = 0;
};

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
    //render_object_id create_static_mesh(const char* name);

    // Destroys a static mesh previously created with create_static_mesh.
    //void destroy_static_mesh(render_object_id id);

    // Set the model that will be rendered on the static mesh.
    //void set_static_mesh_model(render_object_id id, const asset_ptr<model>& model);

    // Sets the material slot of the meshes model to the given material. Overwrites
    // the original material loaded with the model.
    //void set_static_mesh_material(id, slot, mat);

private:
    renderer& m_renderer;

};

}; // namespace ws
