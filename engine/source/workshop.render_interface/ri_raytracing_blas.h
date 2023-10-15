// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/matrix4.h"

namespace ws {

class ri_command_queue;
class ri_buffer;

// ================================================================================================
//  A bottom level acceleration structure for raytracing. This structure essentially contains
//  "triangle soup" that is actually hit-tested against. Its roughly akin to a "model"
// 
//  Instances of this structure can be added/removed from a top level acceleration structure with
//  individual transforms.
// ================================================================================================
class ri_raytracing_blas
{
public:
    virtual ~ri_raytracing_blas() {}

    // Updates the vertex and index buffers used to construct this blas.
    virtual void update(ri_buffer* vertex_buffer, ri_buffer* index_buffer) = 0; 

};

}; // namespace ws
