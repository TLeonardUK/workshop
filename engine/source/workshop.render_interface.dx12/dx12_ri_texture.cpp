// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_texture.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_upload_manager.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"

namespace ws {

dx12_ri_texture::dx12_ri_texture(dx12_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
{
    calculate_formats();
}

dx12_ri_texture::dx12_ri_texture(dx12_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params, Microsoft::WRL::ComPtr<ID3D12Resource> resource, ri_resource_state common_state)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
    , m_handle(resource)
    , m_common_state(common_state)
{
    calculate_formats();
    create_views();
}

dx12_ri_texture::~dx12_ri_texture()
{
    m_renderer.defer_delete([renderer = &m_renderer, handle = m_handle, srv_table = m_srv_table, srv = m_srv, rtv = m_rtv, dsv = m_dsv]()
    {
        if (srv.is_valid())
        {
            renderer->get_descriptor_table(srv_table).free(srv);
        }
        if (rtv.is_valid())
        {
            renderer->get_descriptor_table(ri_descriptor_table::render_target).free(rtv);
        }
        if (dsv.is_valid())
        {
            renderer->get_descriptor_table(ri_descriptor_table::depth_stencil).free(dsv);
        }
        //CheckedRelease(handle);    
    });
}

result<void> dx12_ri_texture::create_resources()
{
    D3D12_RESOURCE_DESC desc;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width = static_cast<UINT64>(m_create_params.width);
    desc.Height = static_cast<UINT>(m_create_params.height);
    desc.MipLevels = static_cast<UINT16>(m_create_params.mip_levels);
    desc.Format = m_resource_format;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    if (m_create_params.dimensions == ri_texture_dimension::texture_cube)
    {
        db_assert(m_create_params.depth == 6);
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.DepthOrArraySize = static_cast<UINT16>(6);
    }
    else
    {
        desc.Dimension = ri_to_dx12(m_create_params.dimensions);
        desc.DepthOrArraySize = static_cast<UINT16>(m_create_params.depth);
    }

    if (m_create_params.is_render_target)
    {
        if (ri_is_format_depth_target(m_create_params.format))
        {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        else
        {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
    }

    if (m_create_params.multisample_count > 0)
    {
        desc.SampleDesc.Count = static_cast<UINT>(m_create_params.multisample_count);
        desc.SampleDesc.Quality = DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN;
    }
    else
    {
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
    }

    D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    m_common_state = ri_resource_state::pixel_shader_resource;

    DXGI_FORMAT clear_format = m_srv_format;

    if (m_create_params.is_render_target)
    {
        if (ri_is_format_depth_target(m_create_params.format))
        {
            initial_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            clear_format = m_dsv_format;
            m_common_state = ri_resource_state::depth_write;
        }
        else
        {
            initial_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
            clear_format = m_rtv_format;
            m_common_state = ri_resource_state::render_target;
        }
    }

    D3D12_CLEAR_VALUE clear_color;
    clear_color.Format = clear_format;
    clear_color.Color[0] = m_create_params.optimal_clear_color.r;
    clear_color.Color[1] = m_create_params.optimal_clear_color.g;
    clear_color.Color[2] = m_create_params.optimal_clear_color.b;
    clear_color.Color[3] = m_create_params.optimal_clear_color.a;
    clear_color.DepthStencil.Depth = m_create_params.optimal_clear_depth;
    clear_color.DepthStencil.Stencil = m_create_params.optimal_clear_stencil;

    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask = 0;
    heap_properties.VisibleNodeMask = 0;

    HRESULT hr = m_renderer.get_device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initial_state,
        m_create_params.is_render_target ? &clear_color : nullptr,
        IID_PPV_ARGS(&m_handle)
    );
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateCommittedResource failed with error 0x%08x.", hr);
        return false;
    }        

    // Upload the linear data if any has been provided.
    if (!m_create_params.data.empty())
    {
        m_renderer.get_upload_manager().upload(
            *this, 
            m_create_params.data
        );
    }

    // Create RTV view if we are to be used as a render target.
    create_views();

    return true;
}

void dx12_ri_texture::calculate_formats()
{
    size_t mip_levels = m_create_params.mip_levels;

    // Calculate formats appropate for this texture.
    m_resource_format = ri_to_dx12(m_create_params.format);
    m_srv_format = m_resource_format;
    m_dsv_format = m_resource_format;
    m_rtv_format = m_resource_format;

    // We use typeless formats for depth as we will specialize with the views.
    if (ri_is_format_depth_target(m_create_params.format))
    {
        if (m_create_params.format == ri_texture_format::D16)
        {
            m_resource_format = DXGI_FORMAT_R16_TYPELESS;
            m_srv_format = DXGI_FORMAT_R16_FLOAT;
            m_dsv_format = DXGI_FORMAT_D16_UNORM;
            m_rtv_format = DXGI_FORMAT_D16_UNORM;
        }
        else if (m_create_params.format == ri_texture_format::D32_FLOAT)
        {
            m_resource_format = DXGI_FORMAT_R32_TYPELESS;
            m_srv_format = DXGI_FORMAT_R32_FLOAT;
            m_dsv_format = DXGI_FORMAT_D32_FLOAT;
            m_rtv_format = DXGI_FORMAT_D32_FLOAT;
        }
        else if (m_create_params.format == ri_texture_format::D24_UNORM_S8_UINT)
        {
            m_resource_format = DXGI_FORMAT_R24G8_TYPELESS;
            m_srv_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            m_dsv_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            m_rtv_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        }
    }

    // Create views for all the view types.
    m_srv_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    m_srv_view_desc.Format = m_srv_format;
    m_dsv_view_desc.Format = m_dsv_format;
    m_rtv_view_desc.Format = m_rtv_format;

    switch (m_create_params.dimensions)
    {
    case ri_texture_dimension::texture_1d:
        {
            m_srv_table = ri_descriptor_table::texture_1d;

            m_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            m_srv_view_desc.Texture1D.MipLevels = static_cast<UINT16>(mip_levels);

            m_dsv_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
            m_dsv_view_desc.Texture1D.MipSlice = 0;
            m_dsv_view_desc.Flags = D3D12_DSV_FLAG_NONE;

            m_rtv_view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
            m_rtv_view_desc.Texture1D.MipSlice = 0;

            break;
        }
    case ri_texture_dimension::texture_2d:
        {
            m_srv_table = ri_descriptor_table::texture_2d;

            m_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            m_srv_view_desc.Texture2D.MipLevels = static_cast<UINT16>(mip_levels);

            m_dsv_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            m_dsv_view_desc.Texture2D.MipSlice = 0;
            m_dsv_view_desc.Flags = D3D12_DSV_FLAG_NONE;

            m_rtv_view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            m_rtv_view_desc.Texture2D.MipSlice = 0;
            m_rtv_view_desc.Texture2D.PlaneSlice = 0;

            break;
        }
    case ri_texture_dimension::texture_3d:
        {
            m_srv_table = ri_descriptor_table::texture_3d;

            m_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            m_srv_view_desc.Texture3D.MipLevels = static_cast<UINT16>(mip_levels);

            break;
        }
    case ri_texture_dimension::texture_cube:
        {
            m_srv_table = ri_descriptor_table::texture_cube;

            m_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            m_srv_view_desc.TextureCube.MipLevels = static_cast<UINT16>(mip_levels);

            break;
        }
    }   

}

void dx12_ri_texture::create_views()
{
    // Set a debug name.
    m_handle->SetName(widen_string(m_debug_name).c_str());

    // Create RTV view if we are to be used as a render target.
    if (m_create_params.is_render_target)
    {
        if (ri_is_format_depth_target(m_create_params.format))
        {
            m_dsv = m_renderer.get_descriptor_table(ri_descriptor_table::depth_stencil).allocate();
            m_renderer.get_device()->CreateDepthStencilView(m_handle.Get(), &m_dsv_view_desc, m_dsv.cpu_handle);
        }
        else
        {
            m_rtv = m_renderer.get_descriptor_table(ri_descriptor_table::render_target).allocate();
            m_renderer.get_device()->CreateRenderTargetView(m_handle.Get(), &m_rtv_view_desc, m_rtv.cpu_handle);
        }
    }

    // Depth targets need to be interpreted differently for srvs.
    m_srv = m_renderer.get_descriptor_table(m_srv_table).allocate();
    m_renderer.get_device()->CreateShaderResourceView(m_handle.Get(), &m_srv_view_desc, m_srv.cpu_handle);
}

dx12_ri_descriptor_table::allocation dx12_ri_texture::get_srv() const
{
    return m_srv;
}

dx12_ri_descriptor_table::allocation dx12_ri_texture::get_rtv() const
{
    return m_rtv;
}

dx12_ri_descriptor_table::allocation dx12_ri_texture::get_dsv() const
{
    return m_dsv;
}

size_t dx12_ri_texture::get_width()
{
    return m_create_params.width;
}

size_t dx12_ri_texture::get_height()
{
    return m_create_params.height;
}

size_t dx12_ri_texture::get_depth()
{
    return m_create_params.depth;
}

size_t dx12_ri_texture::get_mip_levels()
{
    return m_create_params.mip_levels;
}

ri_texture_dimension dx12_ri_texture::get_dimensions() const
{
    return m_create_params.dimensions;
}

ri_texture_format dx12_ri_texture::get_format()
{
    return m_create_params.format;
}

size_t dx12_ri_texture::get_multisample_count()
{
    return m_create_params.multisample_count;
}

color dx12_ri_texture::get_optimal_clear_color()
{
    return m_create_params.optimal_clear_color;
}

float dx12_ri_texture::get_optimal_clear_depth()
{
    return m_create_params.optimal_clear_depth;
}

uint8_t dx12_ri_texture::get_optimal_clear_stencil()
{
    return m_create_params.optimal_clear_stencil;
}

bool dx12_ri_texture::is_render_target()
{
    return m_create_params.is_render_target;
}

bool dx12_ri_texture::is_depth_stencil()
{
    return ri_is_format_depth_target(m_create_params.format);
}

ID3D12Resource* dx12_ri_texture::get_resource()
{
    return m_handle.Get();
}

ri_resource_state dx12_ri_texture::get_initial_state()
{
    return m_common_state;
}

void dx12_ri_texture::swap(ri_texture* other)
{
    dx12_ri_texture* dx12_other = static_cast<dx12_ri_texture*>(other);

    std::swap(m_debug_name, dx12_other->m_debug_name);
    std::swap(m_create_params, dx12_other->m_create_params);

    std::swap(m_srv_table, dx12_other->m_srv_table);
    std::swap(m_common_state, dx12_other->m_common_state);
    
    std::swap(m_handle, dx12_other->m_handle);

    // Recreate our views in the same descriptor slots. This keeps all the handles used elsewhere
    // in param blocks etc valid, no need to replace everything.
    if (m_rtv.is_valid())
    {
        m_renderer.get_device()->CreateRenderTargetView(m_handle.Get(), &m_rtv_view_desc, m_rtv.cpu_handle);
    }
    if (m_dsv.is_valid())
    {
        m_renderer.get_device()->CreateDepthStencilView(m_handle.Get(), &m_dsv_view_desc, m_dsv.cpu_handle);
    }
    if (m_srv.is_valid())
    {
        m_renderer.get_device()->CreateShaderResourceView(m_handle.Get(), &m_srv_view_desc, m_srv.cpu_handle);
    }
}

}; // namespace ws
