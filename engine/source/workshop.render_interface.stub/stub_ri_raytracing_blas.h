// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_raytracing_blas.h"

namespace ws {

// ================================================================================================
//  Stub implementation of a bottom level acceleration structure, performs no actual work.
// ================================================================================================
class stub_ri_raytracing_blas : public ri_raytracing_blas
{
public:
    virtual void update(ri_buffer* vertex_buffer, ri_buffer* index_buffer) override;

};

}; // namespace ws
