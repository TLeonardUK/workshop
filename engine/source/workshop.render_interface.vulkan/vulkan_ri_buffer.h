// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_buffer.h"

#include <string>
#include <vector>

namespace ws {

// ================================================================================================
//  Implementation of a gpu buffer using Vulkan.
// ================================================================================================
class vulkan_ri_buffer : public ri_buffer
{
public:
    vulkan_ri_buffer(const create_params& params, const char* debug_name);

    virtual size_t get_element_count() override;
    virtual size_t get_element_size() override;

    virtual const char* get_debug_name() override;

    virtual ri_resource_state get_initial_state() override;

    virtual void* map(size_t offset, size_t size) override;
    virtual void unmap(void* pointer) override;

private:
    create_params m_params;
    std::string m_debug_name;
    std::vector<uint8_t> m_backing_store;

};

}; // namespace ws
