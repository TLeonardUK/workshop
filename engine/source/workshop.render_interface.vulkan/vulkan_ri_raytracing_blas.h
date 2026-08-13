// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_raytracing_blas.h"

namespace ws {

// ================================================================================================
//  Implementation of a bottom level acceleration structure for Vulkan.
// ================================================================================================
class vulkan_ri_raytracing_blas : public ri_raytracing_blas
{
public:
    virtual void update(ri_buffer* vertex_buffer, ri_buffer* index_buffer) override;

};

}; // namespace ws
