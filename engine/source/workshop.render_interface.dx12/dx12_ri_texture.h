// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include "workshop.render_interface.dx12/dx12_ri_tile_manager.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;
class dx12_ri_param_block;

// ================================================================================================
//  Implementation of a texture buffer using DirectX 12.
// ================================================================================================
class dx12_ri_texture : public ri_texture
{
public:
    dx12_ri_texture(dx12_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params);
    dx12_ri_texture(dx12_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params, Microsoft::WRL::ComPtr<ID3D12Resource> resource, ri_resource_state common_state);
    virtual ~dx12_ri_texture();

    result<void> create_resources();

    virtual size_t get_width() override;
    virtual size_t get_height() override;
    virtual size_t get_depth() override;
    virtual size_t get_mip_levels() override;
    virtual size_t get_dropped_mips() override;
    virtual ri_texture_dimension get_dimensions() const override;
    virtual ri_texture_format get_format() override;
    virtual size_t get_multisample_count() override;
    virtual color get_optimal_clear_color() override;
    virtual float get_optimal_clear_depth() override;
    virtual uint8_t get_optimal_clear_stencil() override;
    virtual bool is_render_target() override;
    virtual bool is_depth_stencil() override;
    virtual bool is_partially_resident() const override;

    virtual size_t get_resident_mips() override;
    virtual void make_mip_resident(size_t mip_index, const std::span<uint8_t>& linear_data) override;
    virtual void make_mip_resident(size_t mip_index, ri_staging_buffer& data_buffer) override;
    virtual void make_mip_non_resident(size_t mip_index) override;
    virtual size_t get_memory_usage_with_residency(size_t mip_count) override;
    virtual bool is_mip_resident(size_t mip_index) override;
    virtual void get_mip_source_data_range(size_t mip_index, size_t& offset, size_t& size) override;
    virtual void begin_mip_residency_change() override;
    virtual void end_mip_residency_change() override;

    virtual ri_resource_state get_initial_state() override;

    virtual const char* get_debug_name() override;

    virtual void swap(ri_texture* other) override;

    // Calculates the data range for an individual mip in the
    // packed source data that the texture_compiler generates.
    bool calculate_linear_data_mip_range(size_t array_index, size_t mip_index, size_t& offset, size_t& size);

    void add_param_block_reference(dx12_ri_param_block* block);
    void remove_param_block_reference(dx12_ri_param_block* block);

public:
    struct mip_residency
    {
        size_t index = 0; 

        bool is_resident = false;
        bool is_packed = false;

        D3D12_TILED_RESOURCE_COORDINATE tile_coordinate;
        D3D12_TILE_REGION_SIZE tile_size;

        dx12_ri_tile_manager::tile_allocation tile_allocation;
    };

    dx12_ri_descriptor_table::allocation get_main_srv() const;
    dx12_ri_descriptor_table::allocation get_srv(size_t slice, size_t mip) const;
    dx12_ri_descriptor_table::allocation get_rtv(size_t slice, size_t mip) const;
    dx12_ri_descriptor_table::allocation get_uav(size_t slice, size_t mip) const;
    dx12_ri_descriptor_table::allocation get_dsv(size_t slice) const;

    ID3D12Resource* get_resource();

    const mip_residency& get_mip_residency(size_t index);

    size_t calculate_resident_mip_used_bytes();

    size_t get_max_resident_mip();

    void create_views();
    void free_views();
    void recreate_views();

    void calculate_dropped_mips();
    void calculate_formats();

private:        
    mip_residency* get_first_packed_mip_residency();

    void update_packed_mip_chain_residency();

public:
    
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    ri_texture::create_params m_create_params;

    std::vector<mip_residency> m_mip_residency;

    dx12_ri_tile_manager::tile_allocation m_packed_mip_tile_allocation;
    bool m_packed_mips_resident = false;    

    bool m_in_mip_residency_change = false;
    bool m_views_pending_recreate = false;

    memory_type m_memory_type;
    std::unique_ptr<memory_allocation> m_memory_allocation_info = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_handle = nullptr;

    D3D12_SHADER_RESOURCE_VIEW_DESC m_main_srv_view_desc = {};
    std::vector<D3D12_DEPTH_STENCIL_VIEW_DESC> m_dsv_view_descs = {};

    std::vector<std::vector<D3D12_RENDER_TARGET_VIEW_DESC>> m_rtv_view_descs = {};
    std::vector<std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC>> m_uav_view_descs = {};
    std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> m_srv_view_descs = {};

    DXGI_FORMAT m_resource_format;
    DXGI_FORMAT m_srv_format;
    DXGI_FORMAT m_dsv_format;
    DXGI_FORMAT m_rtv_format;
    DXGI_FORMAT m_uav_format;

    std::vector<std::vector<dx12_ri_descriptor_table::allocation>> m_rtvs;
    std::vector<std::vector<dx12_ri_descriptor_table::allocation>> m_uavs;
    std::vector<std::vector<dx12_ri_descriptor_table::allocation>> m_srvs;
    std::vector<dx12_ri_descriptor_table::allocation> m_dsvs;
    dx12_ri_descriptor_table::allocation m_main_srv;

    std::mutex m_reference_mutex;
    std::vector<dx12_ri_param_block*> m_referencing_param_blocks;

    ri_descriptor_table m_srv_table;

    ri_resource_state m_common_state;

};

}; // namespace ws
