// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.core/utils/result.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_table.h"
#include <array>
#include <string>

namespace ws {

class engine;
class dx12_render_interface;

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
    virtual ri_texture_dimension get_dimensions() const override;
    virtual ri_texture_format get_format() override;
    virtual size_t get_multisample_count() override;
    virtual color get_optimal_clear_color() override;
    virtual float get_optimal_clear_depth() override;
    virtual uint8_t get_optimal_clear_stencil() override;
    virtual bool is_render_target() override;
    virtual bool is_depth_stencil() override;

    virtual ri_resource_state get_initial_state() override;

    virtual void swap(ri_texture* other) override;

public:
    dx12_ri_descriptor_table::allocation get_srv() const;
    dx12_ri_descriptor_table::allocation get_rtv(size_t slice) const;
    dx12_ri_descriptor_table::allocation get_dsv(size_t slice) const;

    ID3D12Resource* get_resource();

    void calculate_formats();
    void create_views();

private:
    dx12_render_interface& m_renderer;
    std::string m_debug_name;
    ri_texture::create_params m_create_params;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_handle = nullptr;

    D3D12_SHADER_RESOURCE_VIEW_DESC m_srv_view_desc = {};
    std::vector<D3D12_DEPTH_STENCIL_VIEW_DESC> m_dsv_view_descs = {};
    std::vector<D3D12_RENDER_TARGET_VIEW_DESC> m_rtv_view_descs = {};

    DXGI_FORMAT m_resource_format;
    DXGI_FORMAT m_srv_format;
    DXGI_FORMAT m_dsv_format;
    DXGI_FORMAT m_rtv_format;

    std::vector<dx12_ri_descriptor_table::allocation> m_rtvs;
    std::vector<dx12_ri_descriptor_table::allocation> m_dsvs;
    dx12_ri_descriptor_table::allocation m_srv;

    ri_descriptor_table m_srv_table;

    ri_resource_state m_common_state;

};

}; // namespace ws
