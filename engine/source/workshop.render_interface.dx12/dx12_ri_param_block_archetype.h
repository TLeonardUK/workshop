// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_param_block_archetype.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.core/utils/result.h"

#include <memory>

namespace ws {

class dx12_render_interface;
class dx12_ri_layout_factory;
class ri_layout_factory;

// ================================================================================================
//  Implementation of a param block archetype using DirectX 12.
// ================================================================================================
class dx12_ri_param_block_archetype : public ri_param_block_archetype
{
public:
    dx12_ri_param_block_archetype(dx12_render_interface& renderer, const ri_param_block_archetype::create_params& params, const char* debug_name);
    virtual ~dx12_ri_param_block_archetype();

    // Creates the dx12 resources required by this resource.
    result<void> create_resources();

public:
    struct allocation
    {
    public:
        void* cpu_address;
        void* gpu_address;
        size_t size;

        bool is_valid() const;

    private:
        friend class dx12_ri_param_block_archetype;

        size_t pool_index;
        size_t allocation_index;
        bool valid = false;
    };

    virtual std::unique_ptr<ri_param_block> create_param_block() override;

    allocation allocate();
    void free(allocation alloc);

    dx12_ri_layout_factory& get_layout_factory();

    virtual const char* get_name() override;
    virtual const create_params& get_create_params() override;

    void get_table(allocation alloc, size_t& index, size_t& offset);

private:
    void add_page();

private:

    // How many param block elements to allocate per page.
    static constexpr inline size_t k_page_size = 1024;

    // Alignment of each individual instance. Padding will be added to instances to ensure this.
    static constexpr inline size_t k_instance_alignment = 512;

    struct alloc_page
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> handle = nullptr;
        std::vector<size_t> free_list;

        uint8_t* base_address_cpu = nullptr;
        uint8_t* base_address_gpu = nullptr;

        dx12_ri_descriptor_table::allocation srv;
    };

    std::recursive_mutex m_allocation_mutex;

    dx12_render_interface& m_renderer;
    std::string m_debug_name;

    ri_param_block_archetype::create_params m_create_params;


    std::unique_ptr<ri_layout_factory> m_layout_factory;
    size_t m_instance_size;
    size_t m_instance_stride;

    std::vector<alloc_page> m_pages;

};

}; // namespace ws
