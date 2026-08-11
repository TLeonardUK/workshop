// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_query.h"
#include "workshop.render_interface.vulkan/vulkan_ri_query_manager.h"
#include "workshop.core/utils/result.h"

#include <string>

namespace ws {

class vulkan_render_interface;
class vulkan_ri_command_list;

// ================================================================================================
//  Implementation of a gpu query using Vulkan.
// ================================================================================================
class vulkan_ri_query : public ri_query
{
public:
    vulkan_ri_query(vulkan_render_interface& renderer, const char* debug_name, const create_params& params);
    virtual ~vulkan_ri_query();

    result<void> create_resources();

    virtual const char* get_debug_name() override;
    virtual bool are_results_ready() override;
    virtual double get_results() override;

    void begin(vulkan_ri_command_list& list);
    void end(vulkan_ri_command_list& list);

private:
    vulkan_render_interface& m_renderer;
    create_params m_params;
    std::string m_debug_name;

    vulkan_ri_query_manager::query_id m_query_id = vulkan_ri_query_manager::k_invalid_query_id;

};

}; // namespace ws
