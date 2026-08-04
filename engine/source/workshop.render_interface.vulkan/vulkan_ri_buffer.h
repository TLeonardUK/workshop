// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.render_interface.dx12/dx12_ri_small_buffer_allocator.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a gpu buffer using Vulkan.
// ================================================================================================
class vulkan_ri_buffer : public ri_buffer
{
public:
    vulkan_ri_buffer(vulkan_render_interface& renderer, const char* debug_name, const ri_buffer::create_params& params);
    virtual ~vulkan_ri_buffer();

    result<void> create_resources();

    virtual size_t get_element_count() override;
    virtual size_t get_element_size() override;

    virtual const char* get_debug_name() override;

    virtual ri_resource_state get_initial_state() override;

    virtual void* map(size_t offset, size_t size) override;
    virtual void unmap(void* pointer) override;

private:
    vulkan_render_interface& m_renderer;
    std::string m_debug_name;
    ri_buffer::create_params m_create_params;

};

}; // namespace ws
