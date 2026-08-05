// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_query.h"

#include <string>

namespace ws {

// ================================================================================================
//  Implementation of a gpu query using Vulkan.
// ================================================================================================
class vulkan_ri_query : public ri_query
{
public:
    vulkan_ri_query(const create_params& params, const char* debug_name);

    virtual const char* get_debug_name() override;
    virtual bool are_results_ready() override;
    virtual double get_results() override;

private:
    create_params m_params;
    std::string m_debug_name;

};

}; // namespace ws
