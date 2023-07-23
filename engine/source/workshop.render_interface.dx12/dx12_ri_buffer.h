// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  Implementation of a gpu buffer using DirectX 12.
// ================================================================================================
class dx12_ri_buffer : public ri_buffer
{
public:
    dx12_ri_buffer(dx12_render_interface& renderer, const char* debug_name, const ri_buffer::create_params& params);
    virtual ~dx12_ri_buffer();

    result<void> create_resources();

    virtual size_t get_element_count() override;
    virtual size_t get_element_size() override;

    virtual const char* get_debug_name() override;

    virtual ri_resource_state get_initial_state() override;

    virtual void* map(size_t offset, size_t size) override;
    virtual void unmap(void* pointer) override;


public:
    dx12_ri_descriptor_table::allocation get_srv() const;
    dx12_ri_descriptor_table::allocation get_uav() const;

    ID3D12Resource* get_resource();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    ri_buffer::create_params m_create_params;

    ri_resource_state m_common_state;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_handle = nullptr;

    dx12_ri_descriptor_table::allocation m_srv;
    dx12_ri_descriptor_table::allocation m_uav;

    struct mapped_buffer
    {
        size_t offset;
        size_t size;
        std::vector<uint8_t> data;
    };

    std::vector<mapped_buffer> m_buffers;

};

}; // namespace ws
