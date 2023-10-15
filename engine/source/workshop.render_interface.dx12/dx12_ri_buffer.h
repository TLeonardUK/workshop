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

    bool is_small_buffer();

    size_t get_buffer_offset();

    D3D12_GPU_VIRTUAL_ADDRESS get_gpu_address();

public:
    dx12_ri_descriptor_table::allocation get_srv() const;
    dx12_ri_descriptor_table::allocation get_uav() const;

    dx12_ri_small_buffer_allocator::handle get_small_buffer_allocation() const;

    ID3D12Resource* get_resource();

private:
    result<void> create_exclusive_buffer();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    ri_buffer::create_params m_create_params;

    std::unique_ptr<memory_allocation> m_memory_allocation_info = nullptr;

    ri_resource_state m_common_state;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_handle = nullptr;

    dx12_ri_descriptor_table::allocation m_srv;
    dx12_ri_descriptor_table::allocation m_uav;

    bool m_is_small_buffer = false;
    dx12_ri_small_buffer_allocator::handle m_small_buffer_allocation;

    struct mapped_buffer
    {
        size_t offset;
        size_t size;
        std::vector<uint8_t> data;
    };

    std::mutex m_buffers_mutex;
    std::vector<mapped_buffer> m_buffers;

};

}; // namespace ws
