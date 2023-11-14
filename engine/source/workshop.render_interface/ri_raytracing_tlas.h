// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/matrix4.h"

namespace ws {

class ri_command_queue;
class ri_raytracing_blas;
class ri_param_block;
class ri_buffer;

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
    //      domain      matches up with a ray hitgroup in a ri_pipeline to determine what shader is used when rays are tested against this tlas.
    //      opaque      flag that determines if the blas should be treated as transparent or not, this is important to set appropriate for performance.
    //      metadata    param block that contains metadata for this instance. These param blocks have their table/offset indices laid out linearly in get_metadata_buffer().
    virtual instance_id add_instance(ri_raytracing_blas* blas, const matrix4& transform, size_t domain, bool opaque, ri_param_block* metadata, uint32_t mask) = 0;

    // Removes an instances of a blas previously added with add_instance.
    virtual void remove_instance(instance_id id) = 0;

    // Updates the transform of a blas previously inserted with add_instance.
    virtual void update_instance(instance_id id, const matrix4& transform, uint32_t mask) = 0;

    // Returns a buffer that contains a linearly ordered set table index/offset pairs used to reference metadata param blocks passed in by add_instance.
    // This can be indexed into using the InstanceID in the raytracing shader.
    virtual ri_buffer* get_metadata_buffer() = 0;

};

}; // namespace ws
