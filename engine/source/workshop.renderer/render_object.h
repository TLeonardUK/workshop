// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/quat.h"

#include <string>

namespace ws {

// ================================================================================================
//  Base class for all types of objects that exist within the render scene - meshes, views, etc.
// ================================================================================================
class render_object
{
public:
    virtual ~render_object() {};

public:

    // Gets or sets an arbitrary label used to identify this object in the scene.
    void set_name(const std::string& name);
    std::string get_name();

    // Modifies the transform of this object and potentially updates render data.
    virtual void set_local_transform(const vector3& location, const quat& rotation, const vector3& scale);

    // Gets the current transform.
    vector3 get_local_location();
    vector3 get_local_scale();
    quat get_local_rotation();

protected:

    // Name of the effect, use for debugging.
    std::string m_name;

    // Transformation.
    quat    m_local_rotation = quat::identity;
    vector3 m_local_location = vector3::zero;
    vector3 m_local_scale = vector3::one;

};

}; // namespace ws
