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
}

dx12_ri_texture::dx12_ri_texture(dx12_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params, Microsoft::WRL::ComPtr<ID3D12Resource> resource, ri_resource_state common_state)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
    , m_handle(resource)
    , m_common_state(common_state)
{
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
    desc.Format = ri_to_dx12(m_create_params.format);
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

    if (m_create_params.is_render_target)
    {
        if (ri_is_format_depth_target(m_create_params.format))
        {
            initial_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            m_common_state = ri_resource_state::depth_write;
        }
        else
        {
            initial_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
            m_common_state = ri_resource_state::render_target;
        }
    }

    D3D12_CLEAR_VALUE clear_color;
    clear_color.Format = desc.Format;
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
            m_renderer.get_device()->CreateDepthStencilView(m_handle.Get(), nullptr, m_dsv.cpu_handle);
        }
        else
        {
            m_rtv = m_renderer.get_descriptor_table(ri_descriptor_table::render_target).allocate();
            m_renderer.get_device()->CreateRenderTargetView(m_handle.Get(), nullptr, m_rtv.cpu_handle);
        }
    }

    // Create SRV view for buffer.
    // TODO: need to create a UAV if writable
    switch (m_create_params.dimensions)
    {
    case ri_texture_dimension::texture_1d:
        {
            m_srv_table = ri_descriptor_table::texture_1d;
            break;
        }
    case ri_texture_dimension::texture_2d:
        {
            m_srv_table = ri_descriptor_table::texture_2d;
            break;
        }
    case ri_texture_dimension::texture_3d:
        {
            m_srv_table = ri_descriptor_table::texture_3d;
            break;
        }
    case ri_texture_dimension::texture_cube:
        {
            m_srv_table = ri_descriptor_table::texture_cube;
            break;
        }
    }   

    // TODO: We need to figure out how to handle the format for depth buffer reading. 
    if (!ri_is_format_depth_target(m_create_params.format))
    {
        m_srv = m_renderer.get_descriptor_table(m_srv_table).allocate();
        m_renderer.get_device()->CreateShaderResourceView(m_handle.Get(), nullptr, m_srv.cpu_handle);
    }
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

ID3D12Resource* dx12_ri_texture::get_resource()
{
    return m_handle.Get();
}

ri_resource_state dx12_ri_texture::get_initial_state()
{
    return m_common_state;
}

}; // namespace ws
