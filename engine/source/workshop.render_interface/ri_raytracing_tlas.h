// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/matrix4.h"

namespace ws {

class ri_command_queue;
class ri_raytracing_blas;

// ================================================================================================
//  A top level acceleration structure for raytracing. This structure essentially contains
//  a set of "instances" of bottome level acceleration structures that contain the actual 
//  triangle data.
// ================================================================================================
class ri_raytracing_tlas
{
public:

    // Identifier for a blas instance thats been created inside this tlas.
    using instance_id = size_t;

public:
    virtual ~ri_raytracing_tlas() {}

    // Adds a new instances of a blas at the given transform.
    virtual instance_id add_instance(ri_raytracing_blas* blas, const matrix4& transform) = 0;

    // Removes an instances of a blas previously added with add_instance.
    virtual void remove_instance(instance_id id) = 0;

    // Updates the transform of a blas previously inserted with add_instance.
    virtual void update_instance(instance_id id, const matrix4& transform) = 0;

};

}; // namespace ws
