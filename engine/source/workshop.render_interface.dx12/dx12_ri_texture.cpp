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
#include "workshop.core/memory/memory_tracker.h"

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
    m_renderer.defer_delete([renderer = &m_renderer, handle = m_handle, srv_table = m_srv_table, main_srv = m_main_srv, srvs = m_srvs, rtvs = m_rtvs, uavs = m_uavs, dsvs = m_dsvs]()
    {
        if (main_srv.is_valid())
        {
            renderer->get_descriptor_table(srv_table).free(main_srv);
        }
        for (auto& mip_srvs : srvs)
        {
            for (auto& srv : mip_srvs)
            {
                if (srv.is_valid())
                {
                    renderer->get_descriptor_table(ri_descriptor_table::texture_2d).free(srv);
                }
            }
        }
        for (auto& mip_rtvs : rtvs)
        {
            for (auto& rtv : mip_rtvs)
            {
                if (rtv.is_valid())
                {
                    renderer->get_descriptor_table(ri_descriptor_table::render_target).free(rtv);
                }
            }
        }
        for (auto& mip_uavs : uavs)
        {
            for (auto& uav : mip_uavs)
            {
                if (uav.is_valid())
                {
                    renderer->get_descriptor_table(ri_descriptor_table::rwtexture_2d).free(uav);
                }
            }
        }
        for (auto& dsv : dsvs)
        {
            if (dsv.is_valid())
            {
                renderer->get_descriptor_table(ri_descriptor_table::depth_stencil).free(dsv);
            }
        }
        //CheckedRelease(handle);    
    });
}

result<void> dx12_ri_texture::create_resources()
{
    memory_type mem_type = memory_type::rendering__vram__texture;
    if (m_create_params.is_render_target)
    {
        if (ri_is_format_depth_target(m_create_params.format))
        {
            mem_type = memory_type::rendering__vram__render_target_depth;
        }
        else
        {
            mem_type = memory_type::rendering__vram__render_target_color;
        }
    }

    memory_scope mem_scope(mem_type, string_hash::empty, string_hash(m_debug_name));

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

    if (m_create_params.allow_unordered_access)
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
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
    if (is_depth_stencil())
    {
        clear_color.DepthStencil.Depth = m_create_params.optimal_clear_depth;
        clear_color.DepthStencil.Stencil = m_create_params.optimal_clear_stencil;
    }
    else
    {
        clear_color.Color[0] = m_create_params.optimal_clear_color.r;
        clear_color.Color[1] = m_create_params.optimal_clear_color.g;
        clear_color.Color[2] = m_create_params.optimal_clear_color.b;
        clear_color.Color[3] = m_create_params.optimal_clear_color.a;
    }

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

    // Record the memory allocation.
    D3D12_RESOURCE_ALLOCATION_INFO info = m_renderer.get_device()->GetResourceAllocationInfo(0, 1, &desc);
    m_memory_allocation_info = mem_scope.record_alloc(info.SizeInBytes);

#if 1
    if (mem_type == memory_type::rendering__vram__texture)
    {
        db_log(renderer, "[TEXTURE] %s [%zi x %zi] %zi kb",m_debug_name.c_str(), m_create_params.width, m_create_params.height, info.SizeInBytes / 1024); 
    }
#endif

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
    // If we've been provided raw data then calculate how many mips to drop if requested.
    if (!m_create_params.data.empty() && m_create_params.drop_mips > 0)
    {
        // Try and drop as many mips as requested.
        size_t to_drop = m_create_params.drop_mips;
        m_create_params.drop_mips = 0;

        while (m_create_params.width >= 4 &&
            m_create_params.height >= 4 &&
            m_create_params.mip_levels >= 2 &&
            to_drop > 0)
        {
            m_create_params.width /= 2;
            m_create_params.height /= 2;
            m_create_params.drop_mips++;
            m_create_params.mip_levels--;
            to_drop--;
        }
    }
    else
    {
        m_create_params.drop_mips = 0;
    }

    size_t mip_levels = m_create_params.mip_levels;

    // Calculate formats appropate for this texture.
    m_resource_format = ri_to_dx12(m_create_params.format);
    m_srv_format = m_resource_format;
    m_dsv_format = m_resource_format;
    m_rtv_format = m_resource_format;
    m_uav_format = m_resource_format;

    // We use typeless formats for depth as we will specialize with the views.
    if (ri_is_format_depth_target(m_create_params.format))
    {
        if (m_create_params.format == ri_texture_format::D16)
        {
            m_resource_format = DXGI_FORMAT_R16_TYPELESS;
            m_srv_format = DXGI_FORMAT_R16_FLOAT;
            m_dsv_format = DXGI_FORMAT_D16_UNORM;
            m_rtv_format = DXGI_FORMAT_D16_UNORM;
            m_uav_format = DXGI_FORMAT_R16_FLOAT;
        }
        else if (m_create_params.format == ri_texture_format::D32_FLOAT)
        {
            m_resource_format = DXGI_FORMAT_R32_TYPELESS;
            m_srv_format = DXGI_FORMAT_R32_FLOAT;
            m_dsv_format = DXGI_FORMAT_D32_FLOAT;
            m_rtv_format = DXGI_FORMAT_D32_FLOAT;
            m_uav_format = DXGI_FORMAT_D32_FLOAT;
        }
        else if (m_create_params.format == ri_texture_format::D24_UNORM_S8_UINT)
        {
            m_resource_format = DXGI_FORMAT_R24G8_TYPELESS;
            m_srv_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            m_dsv_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            m_rtv_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            m_uav_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        }
    }

    // Create views for all the view types.
    m_main_srv_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_main_srv_view_desc.Format = m_srv_format;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_template_desc;
    dsv_template_desc.Format = m_dsv_format;

    D3D12_RENDER_TARGET_VIEW_DESC rtv_template_desc;
    rtv_template_desc.Format = m_rtv_format;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_template_desc;
    uav_template_desc.Format = m_uav_format;

    switch (m_create_params.dimensions)
    {
    case ri_texture_dimension::texture_1d:
        {
            db_assert(!m_create_params.allow_unordered_access);
            db_assert(!m_create_params.allow_individual_image_access);

            m_srv_table = ri_descriptor_table::texture_1d;

            m_main_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            m_main_srv_view_desc.Texture1D.MipLevels = static_cast<UINT16>(mip_levels);
            m_main_srv_view_desc.Texture1D.MostDetailedMip = 0;
            m_main_srv_view_desc.Texture1D.ResourceMinLODClamp = 0;

            dsv_template_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
            dsv_template_desc.Texture1D.MipSlice = 0;
            dsv_template_desc.Flags = D3D12_DSV_FLAG_NONE;

            rtv_template_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
            rtv_template_desc.Texture1D.MipSlice = 0;

            uav_template_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uav_template_desc.Texture1D.MipSlice = 0;

            m_rtv_view_descs.resize(mip_levels);
            m_rtvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                rtv_template_desc.Texture1D.MipSlice = mip;

                m_rtv_view_descs[mip].push_back(rtv_template_desc);
                m_rtvs[mip].resize(1);
            }

            m_dsv_view_descs.push_back(dsv_template_desc);
            m_dsvs.resize(1);

            break;
        }
    case ri_texture_dimension::texture_2d:
        {
            m_srv_table = ri_descriptor_table::texture_2d;

            m_main_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            m_main_srv_view_desc.Texture2D.MipLevels = static_cast<UINT16>(mip_levels);
            m_main_srv_view_desc.Texture2D.MostDetailedMip = 0;
            m_main_srv_view_desc.Texture2D.ResourceMinLODClamp = 0;
            m_main_srv_view_desc.Texture2D.PlaneSlice = 0;

            dsv_template_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv_template_desc.Texture2D.MipSlice = 0;
            dsv_template_desc.Flags = D3D12_DSV_FLAG_NONE;

            rtv_template_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtv_template_desc.Texture2D.MipSlice = 0;
            rtv_template_desc.Texture2D.PlaneSlice = 0;

            uav_template_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uav_template_desc.Texture2D.MipSlice = 0;
            uav_template_desc.Texture2D.PlaneSlice = 0;

            m_rtv_view_descs.resize(mip_levels);
            m_rtvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                rtv_template_desc.Texture2D.MipSlice = mip;

                m_rtv_view_descs[mip].push_back(rtv_template_desc);
                m_rtvs[mip].resize(1);
            }

            m_uav_view_descs.resize(mip_levels);
            m_uavs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                uav_template_desc.Texture2D.MipSlice = mip;

                m_uav_view_descs[mip].push_back(uav_template_desc);
                m_uavs[mip].resize(1);
            }

            m_srv_view_descs.resize(mip_levels);
            m_srvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc = m_main_srv_view_desc;
                desc.Texture2D.MipLevels = 1;
                desc.Texture2D.MostDetailedMip = mip;

                m_srv_view_descs[mip].push_back(desc);
                m_srvs[mip].resize(1);
            }

            m_dsv_view_descs.push_back(dsv_template_desc);
            m_dsvs.resize(1);

            break;
        }
    case ri_texture_dimension::texture_3d:
        {
            m_srv_table = ri_descriptor_table::texture_3d;

            m_main_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            m_main_srv_view_desc.Texture3D.MipLevels = static_cast<UINT16>(mip_levels);
            m_main_srv_view_desc.Texture3D.MostDetailedMip = 0;
            m_main_srv_view_desc.Texture3D.ResourceMinLODClamp = 0;

            dsv_template_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsv_template_desc.Texture2DArray.MipSlice = 0;
            dsv_template_desc.Texture2DArray.FirstArraySlice = 0;
            dsv_template_desc.Texture2DArray.ArraySize = 1;
            dsv_template_desc.Flags = D3D12_DSV_FLAG_NONE;

            rtv_template_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtv_template_desc.Texture2DArray.MipSlice = 0;
            rtv_template_desc.Texture2DArray.FirstArraySlice = 0;
            rtv_template_desc.Texture2DArray.PlaneSlice = 0;

            uav_template_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uav_template_desc.Texture2DArray.MipSlice = 0;
            uav_template_desc.Texture2DArray.FirstArraySlice = 0;
            uav_template_desc.Texture2DArray.PlaneSlice = 0;

            m_dsvs.resize(m_create_params.depth);
            for (size_t i = 0; i < m_create_params.depth; i++)
            {
                dsv_template_desc.Texture2DArray.FirstArraySlice = i;
                m_dsv_view_descs.push_back(dsv_template_desc);
            }

            m_rtv_view_descs.resize(mip_levels);
            m_rtvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                m_rtvs[mip].resize(m_create_params.depth);
                for (size_t i = 0; i < m_create_params.depth; i++)
                {
                    rtv_template_desc.Texture2DArray.PlaneSlice = i;
                    rtv_template_desc.Texture2DArray.MipSlice = mip;
                    m_rtv_view_descs[mip].push_back(rtv_template_desc);
                }
            }

            m_uav_view_descs.resize(mip_levels);
            m_uavs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                m_uavs[mip].resize(m_create_params.depth);
                for (size_t i = 0; i < m_create_params.depth; i++)
                {
                    uav_template_desc.Texture2DArray.PlaneSlice = i;
                    uav_template_desc.Texture2DArray.MipSlice = mip;
                    m_uav_view_descs[mip].push_back(uav_template_desc);
                }
            }

            m_srv_view_descs.resize(mip_levels);
            m_srvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                m_srvs[mip].resize(m_create_params.depth);
                for (size_t i = 0; i < m_create_params.depth; i++)
                {                    
                    D3D12_SHADER_RESOURCE_VIEW_DESC desc = m_main_srv_view_desc;
                    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.MostDetailedMip = mip;
                    desc.Texture2DArray.MipLevels = 1;
                    desc.Texture2DArray.PlaneSlice = i;

                    rtv_template_desc.Texture2DArray.PlaneSlice = i;
                    rtv_template_desc.Texture2DArray.MipSlice = mip;
                    m_srv_view_descs[mip].push_back(desc);
                }
            }

            break;
        }
    case ri_texture_dimension::texture_cube:
        {
            m_srv_table = ri_descriptor_table::texture_cube;

            m_main_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            m_main_srv_view_desc.TextureCube.MipLevels = static_cast<UINT16>(mip_levels);
            m_main_srv_view_desc.TextureCube.MostDetailedMip = 0;
            m_main_srv_view_desc.TextureCube.ResourceMinLODClamp = 0;

            dsv_template_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsv_template_desc.Texture2DArray.MipSlice = 0;
            dsv_template_desc.Texture2DArray.FirstArraySlice = 0;
            dsv_template_desc.Texture2DArray.ArraySize = 1;
            dsv_template_desc.Flags = D3D12_DSV_FLAG_NONE;

            rtv_template_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtv_template_desc.Texture2DArray.MipSlice = 0;
            rtv_template_desc.Texture2DArray.PlaneSlice = 0;
            rtv_template_desc.Texture2DArray.FirstArraySlice = 0;
            rtv_template_desc.Texture2DArray.ArraySize = 1;

            uav_template_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uav_template_desc.Texture2DArray.MipSlice = 0;
            uav_template_desc.Texture2DArray.PlaneSlice = 0;
            uav_template_desc.Texture2DArray.FirstArraySlice = 0;
            uav_template_desc.Texture2DArray.ArraySize = 1;

            m_dsvs.resize(m_create_params.depth);
            for (size_t i = 0; i < m_create_params.depth; i++)
            {
                dsv_template_desc.Texture2DArray.FirstArraySlice = i;
                m_dsv_view_descs.push_back(dsv_template_desc);
            }

            m_rtv_view_descs.resize(mip_levels);
            m_rtvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                m_rtvs[mip].resize(m_create_params.depth);
                for (size_t i = 0; i < m_create_params.depth; i++)
                {
                    rtv_template_desc.Texture2DArray.FirstArraySlice = i;
                    rtv_template_desc.Texture2DArray.MipSlice = mip;
                    m_rtv_view_descs[mip].push_back(rtv_template_desc);
                }
            }

            m_uav_view_descs.resize(mip_levels);
            m_uavs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                m_uavs[mip].resize(m_create_params.depth);
                for (size_t i = 0; i < m_create_params.depth; i++)
                {
                    uav_template_desc.Texture2DArray.FirstArraySlice = i;
                    uav_template_desc.Texture2DArray.MipSlice = mip;
                    m_uav_view_descs[mip].push_back(uav_template_desc);
                }
            }

            m_srv_view_descs.resize(mip_levels);
            m_srvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                m_srvs[mip].resize(m_create_params.depth);
                for (size_t i = 0; i < m_create_params.depth; i++)
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC desc = m_main_srv_view_desc;
                    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.MostDetailedMip = mip;
                    desc.Texture2DArray.MipLevels = 1;
                    desc.Texture2DArray.PlaneSlice = 0;
                    desc.Texture2DArray.FirstArraySlice = i;
                    desc.Texture2DArray.ArraySize = 1;

                    rtv_template_desc.Texture2DArray.PlaneSlice = i;
                    rtv_template_desc.Texture2DArray.MipSlice = mip;
                    m_srv_view_descs[mip].push_back(desc);
                }
            }

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
            for (size_t i = 0; i < m_dsvs.size(); i++)
            {
                m_dsvs[i] = m_renderer.get_descriptor_table(ri_descriptor_table::depth_stencil).allocate();
                m_renderer.get_device()->CreateDepthStencilView(m_handle.Get(), &m_dsv_view_descs[i], m_dsvs[i].cpu_handle);
            }
        }
        else
        {
            for (size_t mip = 0; mip < get_mip_levels(); mip++)
            {
                for (size_t slice = 0; slice < m_rtvs[mip].size(); slice++)
                {
                    m_rtvs[mip][slice] = m_renderer.get_descriptor_table(ri_descriptor_table::render_target).allocate();
                    m_renderer.get_device()->CreateRenderTargetView(m_handle.Get(), &m_rtv_view_descs[mip][slice], m_rtvs[mip][slice].cpu_handle);
                }
            }
        }
    }

    if (m_create_params.allow_unordered_access)
    {
        for (size_t mip = 0; mip < get_mip_levels(); mip++)
        {
            for (size_t slice = 0; slice < m_uavs[mip].size(); slice++)
            {
                m_uavs[mip][slice] = m_renderer.get_descriptor_table(ri_descriptor_table::rwtexture_2d).allocate();
                m_renderer.get_device()->CreateUnorderedAccessView(m_handle.Get(), nullptr, &m_uav_view_descs[mip][slice], m_uavs[mip][slice].cpu_handle);
            }
        }
    }

    if (m_create_params.allow_individual_image_access)
    {
        for (size_t mip = 0; mip < get_mip_levels(); mip++)
        {
            for (size_t slice = 0; slice < m_srvs[mip].size(); slice++)
            {
                m_srvs[mip][slice] = m_renderer.get_descriptor_table(ri_descriptor_table::texture_2d).allocate();
                m_renderer.get_device()->CreateShaderResourceView(m_handle.Get(), &m_srv_view_descs[mip][slice], m_srvs[mip][slice].cpu_handle);
            }
        }
    }

    // Depth targets need to be interpreted differently for srvs.
    m_main_srv = m_renderer.get_descriptor_table(m_srv_table).allocate();
    m_renderer.get_device()->CreateShaderResourceView(m_handle.Get(), &m_main_srv_view_desc, m_main_srv.cpu_handle);
}

dx12_ri_descriptor_table::allocation dx12_ri_texture::get_main_srv() const
{
    return m_main_srv;
}

dx12_ri_descriptor_table::allocation dx12_ri_texture::get_srv(size_t slice, size_t mip) const
{
    db_assert(m_create_params.allow_individual_image_access);
    return m_srvs[mip][slice];
}

dx12_ri_descriptor_table::allocation dx12_ri_texture::get_rtv(size_t slice, size_t mip) const
{
    return m_rtvs[mip][slice];
}

dx12_ri_descriptor_table::allocation dx12_ri_texture::get_uav(size_t slice, size_t mip) const
{
    db_assert(m_create_params.allow_unordered_access);
    return m_uavs[mip][slice];
}

dx12_ri_descriptor_table::allocation dx12_ri_texture::get_dsv(size_t slice) const
{
    return m_dsvs[slice];
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

size_t dx12_ri_texture::get_dropped_mips()
{
    return m_create_params.drop_mips;
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
    for (size_t mip = 0; mip < get_mip_levels(); mip++)
    {
        for (size_t slice = 0; slice < m_rtvs[mip].size(); slice++)
        {
            if (m_rtvs[mip][slice].is_valid())
            {
                m_renderer.get_device()->CreateRenderTargetView(m_handle.Get(), &m_rtv_view_descs[mip][slice], m_rtvs[mip][slice].cpu_handle);
            }
        }
    }
    for (size_t mip = 0; mip < get_mip_levels(); mip++)
    {
        for (size_t slice = 0; slice < m_uavs[mip].size(); slice++)
        {
            if (m_uavs[mip][slice].is_valid())
            {
                m_renderer.get_device()->CreateUnorderedAccessView(m_handle.Get(), nullptr, &m_uav_view_descs[mip][slice], m_uavs[mip][slice].cpu_handle);
            }
        }
    }
    for (size_t i = 0; i < m_dsvs.size(); i++)
    {
        if (m_dsvs[i].is_valid())
        {
            m_renderer.get_device()->CreateDepthStencilView(m_handle.Get(), &m_dsv_view_descs[i], m_dsvs[i].cpu_handle);
        }
    }
    for (size_t mip = 0; mip < get_mip_levels(); mip++)
    {
        for (size_t slice = 0; slice < m_srvs[mip].size(); slice++)
        {
            if (m_srvs[mip][slice].is_valid())
            {
                m_renderer.get_device()->CreateShaderResourceView(m_handle.Get(), &m_srv_view_descs[mip][slice], m_srvs[mip][slice].cpu_handle);
            }
        }
    }
    if (m_main_srv.is_valid())
    {
        m_renderer.get_device()->CreateShaderResourceView(m_handle.Get(), &m_main_srv_view_desc, m_main_srv.cpu_handle);
    }
}

}; // namespace ws
