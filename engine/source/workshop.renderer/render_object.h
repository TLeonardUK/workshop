// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/matrix4.h"
#include "workshop.core/math/obb.h"
#include "workshop.core/containers/oct_tree.h"
#include "workshop.renderer/common_types.h"
#include "workshop.renderer/render_command_queue.h"
#include "workshop.renderer/render_visibility_manager.h"

#include <string>
#include <bitset>

namespace ws {

class render_scene_manager;
enum class render_gpu_flags;

// ================================================================================================
//  Base class for all types of objects that exist within the render scene - meshes, views, etc.
// ================================================================================================
class render_object
{
public:
    render_object(render_object_id id, renderer* renderer, render_visibility_flags visibility_flags);
    virtual ~render_object();

public:

    // Simple function called after the constructor to do any setup required that cannot occur in the constructor.
    virtual void init();

    // Gets or sets an arbitrary label used to identify this object in the scene.
    void set_name(const std::string& name);
    std::string get_name();

    // Gets the id of this object as used to reference it via the scene manager.
    render_object_id get_id();

    // Modifies the transform of this object and potentially updates render data.
    virtual void set_local_transform(const vector3& location, const quat& rotation, const vector3& scale);

    // Gets the current transform.
    vector3 get_local_location();
    vector3 get_local_scale();
    quat get_local_rotation();

    matrix4 get_transform();

    // Gets the bounds of this object in world space.
    virtual obb get_bounds();

    // Called when the bounds of an object is modified.
    virtual void bounds_modified();

    // Gets the id of this object in the visibility system.
    render_visibility_manager::object_id get_visibility_id();

    // Sets the render flags of this object.
    virtual void set_render_gpu_flags(render_gpu_flags flags);
    render_gpu_flags get_render_gpu_flags();

protected:

    bool m_store_in_octtree = false;

    render_object_id m_id;

    render_gpu_flags m_gpu_flags = (render_gpu_flags)0;

    // Visibility manager state.
    render_visibility_manager::object_id m_visibility_id;
    render_visibility_flags m_visibility_flags;

    // Renderer thats using us.
    renderer* m_renderer;

    // Name of the effect, use for debugging.
    std::string m_name;

    // Transformation.
    quat    m_local_rotation = quat::identity;
    vector3 m_local_location = vector3::zero;
    vector3 m_local_scale = vector3::one;

};

}; // namespace ws
