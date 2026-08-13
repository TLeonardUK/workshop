// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_pipeline.h"

namespace ws {

// ================================================================================================
//  Implementation of a pipeline using Vulkan.
// ================================================================================================
class vulkan_ri_pipeline : public ri_pipeline
{
public:
    vulkan_ri_pipeline(const create_params& params, const char* debug_name);

    virtual const create_params& get_create_params() override;

private:
    create_params m_params;

};

}; // namespace ws
