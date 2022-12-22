// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"

#include "workshop.render_interface/ri_types.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_heap.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>

namespace ws {

class engine;
class dx12_render_interface;

// ================================================================================================
//  The descriptor tables take a chunk of allocations out of one of the descriptor heap's and 
//  sub-allocate them out to anything that asks for them.
// 
//  Each table allocates for a specific resource-type. These descriptor tables are then bound to 
//  the different unbound arrays in our shaders.
// 
//  They are essentially a single bindless array of resources. 
// ================================================================================================
class dx12_ri_descriptor_table
{
public:
    struct allocation
    {
    public:
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;

        bool is_valid() const;
        size_t get_table_index() const;

    private:
        friend class dx12_ri_descriptor_table;

        bool valid = false;
        size_t index;
    };

public:
    dx12_ri_descriptor_table(dx12_render_interface& renderer, ri_descriptor_table table_type);
    virtual ~dx12_ri_descriptor_table();

    // Creates the dx12 resources required by this swapchain.
    result<void> create_resources();

    allocation allocate();
    void free(allocation handle);

    allocation get_base_allocation();

private:
    std::mutex m_mutex;

    dx12_render_interface& m_renderer;
    dx12_ri_descriptor_heap* m_heap;
    
    ri_descriptor_table m_table_type;
    size_t m_size;

    dx12_ri_descriptor_heap::allocation m_allocation;

    std::vector<size_t> m_free_list;

};

}; // namespace ws
