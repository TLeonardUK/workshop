// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_texture.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_upload_manager.h"
#include "workshop.render_interface.dx12/dx12_ri_tile_manager.h"
#include "workshop.render_interface.dx12/dx12_ri_param_block.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"
#include "workshop.core/memory/memory_tracker.h"

#pragma optimize("", off)

namespace ws {

dx12_ri_texture::dx12_ri_texture(dx12_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
{
    calculate_dropped_mips();
    calculate_formats();
}

dx12_ri_texture::dx12_ri_texture(dx12_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params, Microsoft::WRL::ComPtr<ID3D12Resource> resource, ri_resource_state common_state)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
    , m_handle(resource)
    , m_common_state(common_state)
{
    calculate_dropped_mips();
    calculate_formats();
    create_views();
}

dx12_ri_texture::~dx12_ri_texture()
{
    // If any param blocks are still referencing this texture, tell them to remove their references.
    {
        std::scoped_lock lock(m_reference_mutex);

        for (dx12_ri_param_block* ref : m_referencing_param_blocks)
        {
            ref->clear_texture_references(this);
        }
    }

    // Deallocate all tiles (these are already defered, we don't need to put it in defer_delete).
    if (m_create_params.is_partially_resident)
    {
        dx12_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();
        
        if (m_packed_mips_resident)
        {
            tile_manager.free_tiles(m_packed_mip_tile_allocation);
            m_packed_mip_tile_allocation = {};
        }

        for (mip_residency& mip : m_mip_residency)
        {
            if (mip.is_resident && !mip.is_packed)
            {
                tile_manager.free_tiles(mip.tile_allocation);
                mip.tile_allocation = {};
                mip.is_resident = false;
            }
        }
    }

    free_views();

    m_renderer.defer_delete([handle = m_handle]()
    {
        // Don't need to do anything here, this is just here to maintain a handle reference.
    });
}

void dx12_ri_texture::add_param_block_reference(dx12_ri_param_block* block)
{
    std::scoped_lock lock(m_reference_mutex);

    m_referencing_param_blocks.push_back(block);
}

void dx12_ri_texture::remove_param_block_reference(dx12_ri_param_block* block)
{
    std::scoped_lock lock(m_reference_mutex);

    if (auto iter = std::find(m_referencing_param_blocks.begin(), m_referencing_param_blocks.end(), block); iter != m_referencing_param_blocks.end())
    {
        m_referencing_param_blocks.erase(iter);
    }
}

bool dx12_ri_texture::calculate_linear_data_mip_range(size_t array_index, size_t mip_index, size_t& offset, size_t& size)
{
    // TODO: Grim, do all this better.

    size_t block_size = ri_format_block_size(m_create_params.format);
    size_t face_count = m_create_params.depth;
    size_t slice_count = m_create_params.depth;
    size_t mip_count = m_create_params.mip_levels;
    size_t dropped_mip_count = m_create_params.drop_mips;
    size_t array_count = 1;
    if (m_create_params.dimensions == ri_texture_dimension::texture_cube)
    {
        face_count = 6;
        array_count = 6;
        slice_count = 1;
    }

    size_t undropped_width = m_create_params.width;
    size_t undropped_height = m_create_params.height;
    for (size_t i = 0; i < dropped_mip_count; i++)
    {
        undropped_width *= 2;
        undropped_height *= 2;
    }

    size_t data_offset = 0;
    for (size_t face = 0; face < face_count; face++)
    {
        size_t mip_width = undropped_width;
        size_t mip_height = undropped_height;

        for (size_t mip = 0; mip < mip_count + dropped_mip_count; mip++)
        {
            const size_t mip_size = (ri_bytes_per_texel(m_create_params.format) * mip_width * mip_height) / block_size;

            if (mip == (mip_index + dropped_mip_count) && array_index == face)
            {
                offset = data_offset;
                size = mip_size;
                return true;
            }

            data_offset += mip_size;

            mip_width = std::max(1llu, mip_width / 2);
            mip_height = std::max(1llu, mip_height / 2);
        }
    }

    return false;
}

result<void> dx12_ri_texture::create_resources()
{
    m_memory_type = memory_type::rendering__vram__texture;
    if (m_create_params.is_render_target)
    {
        if (ri_is_format_depth_target(m_create_params.format))
        {
            m_memory_type = memory_type::rendering__vram__render_target_depth;
        }
        else
        {
            m_memory_type = memory_type::rendering__vram__render_target_color;
        }
    }

    memory_scope mem_scope(m_memory_type, string_hash::empty, string_hash(m_debug_name));

    db_assert_message(!m_create_params.is_partially_resident || m_create_params.dimensions == ri_texture_dimension::texture_2d, "Only 2d textures supported partial residency (for now).");

    D3D12_RESOURCE_DESC desc;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width = static_cast<UINT64>(m_create_params.width);
    desc.Height = static_cast<UINT>(m_create_params.height);
    desc.MipLevels = static_cast<UINT16>(m_create_params.mip_levels);
    desc.Format = m_resource_format;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (m_create_params.is_partially_resident)
    {
        desc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
    }
    else
    {
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    }

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

    // Create streamed textures as partially resident.
    if (m_create_params.is_partially_resident)
    {
        HRESULT hr = m_renderer.get_device()->CreateReservedResource(
            &desc,
            initial_state,
            m_create_params.is_render_target ? &clear_color : nullptr,
            IID_PPV_ARGS(&m_handle)
        );

        if (FAILED(hr))
        {
            db_error(render_interface, "CreateReservedResource failed with error 0x%08x.", hr);
            return false;
        }

        // Calculate tiling information for resource.
        UINT total_tiles = 0;
        D3D12_TILE_SHAPE tile_shape = {};
        D3D12_PACKED_MIP_INFO packed_mip_info;
        UINT subresource_count = (UINT)m_create_params.mip_levels;
        std::vector<D3D12_SUBRESOURCE_TILING> subresource_tiling(subresource_count);

        m_renderer.get_device()->GetResourceTiling(m_handle.Get(), &total_tiles, &packed_mip_info, &tile_shape, &subresource_count, 0, subresource_tiling.data());

        // Store information on all the mips.
        m_mip_residency.resize(m_create_params.mip_levels);

        for (size_t i = 0; i < m_create_params.mip_levels; i++)
        {
            D3D12_SUBRESOURCE_TILING& tiling = subresource_tiling[i];

            mip_residency& residency = m_mip_residency[i];
            residency.index = i;
            residency.is_resident = false;
            residency.tile_coordinate = { 0, 0, 0, (UINT)i };

            if (i < packed_mip_info.NumStandardMips)
            {
                residency.is_packed = false;
                residency.tile_size.Width = tiling.WidthInTiles;
                residency.tile_size.Height = tiling.HeightInTiles;
                residency.tile_size.Depth = tiling.DepthInTiles;
                residency.tile_size.UseBox = true;
                residency.tile_size.NumTiles = tiling.WidthInTiles * tiling.HeightInTiles * tiling.DepthInTiles;
            }
            else
            {
                residency.is_packed = true;
                residency.tile_size.UseBox = false;
                residency.tile_size.NumTiles = packed_mip_info.NumTilesForPackedMips;
            }
        }    
        
        // Make initial mips resident.
        for (size_t i = 0; i < m_create_params.resident_mips; i++)
        {
            std::vector<uint8_t> mip_data;
            size_t mip_index = m_create_params.mip_levels - (i + 1);
            size_t mip_offset = 0;
            size_t mip_size = 0;

            if (!m_create_params.data.empty())
            {
                calculate_linear_data_mip_range(0, mip_index, mip_offset, mip_size);
                //db_log(renderer, "mip[%zi] offset=%zi size=%zi path=%s", mip_index, mip_offset, mip_size, m_debug_name.c_str());
                mip_data.assign(m_create_params.data.begin() + mip_offset, m_create_params.data.begin() + mip_offset + mip_size);
            }

            make_mip_resident(mip_index, mip_data);
        }

        m_memory_allocation_info = mem_scope.record_alloc(calculate_resident_mip_used_bytes());

        // Recalculate view formats so the resident mip cap is set.
        calculate_formats();
    }
    else
    {
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

        // Upload the linear data if any has been provided.
        if (!m_create_params.data.empty())
        {
            m_renderer.get_upload_manager().upload(
                *this,
                m_create_params.data
            );
        }
    }

    // Create RTV view if we are to be used as a render target.
    create_views();

    return true;
}

const dx12_ri_texture::mip_residency& dx12_ri_texture::get_mip_residency(size_t index)
{
    db_assert(m_create_params.is_partially_resident);
    return m_mip_residency[index];
}

size_t dx12_ri_texture::get_max_resident_mip()
{
    if (!m_create_params.is_partially_resident)
    {
        return 0;
    }

    // Mip residency hasn't been calculated yet, so return zero.
    if (m_mip_residency.empty())
    {
        return 0;
    }

    // Look for the maximum mip where all the precending mips are also resident.
    for (int32_t i = (int32_t)m_mip_residency.size() - 1; i >= 0; i--)
    {
        if (!m_mip_residency[i].is_resident)
        {
            return i + 1;
        }
    }

    return 0;
}

dx12_ri_texture::mip_residency* dx12_ri_texture::get_first_packed_mip_residency()
{
    for (size_t i = 0; i < m_mip_residency.size(); i++)
    {
        if (m_mip_residency[i].is_packed)
        {
            return &m_mip_residency[i];
        }
    }
    return nullptr;
}

size_t dx12_ri_texture::calculate_resident_mip_used_bytes()
{
    size_t total = 0;
    bool added_packed_tiles = false;

    for (mip_residency& mip : m_mip_residency)
    {
        if (mip.is_resident)
        {
            if (!mip.is_packed || !added_packed_tiles)
            {
                total += mip.tile_size.NumTiles * D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
                if (mip.is_packed)
                {
                    added_packed_tiles = true;
                }
            }
        }
    }

    return total;
}

void dx12_ri_texture::update_packed_mip_chain_residency()
{
    dx12_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();

    bool should_packed_mips_be_resident = false;
    size_t packed_tile_count = 0;
    size_t first_packed_mip_index = std::numeric_limits<size_t>::max();

    for (mip_residency& mip : m_mip_residency)
    {
        if (mip.is_resident && mip.is_packed)
        {
            should_packed_mips_be_resident = true;
            packed_tile_count = mip.tile_size.NumTiles;
        }

        if (mip.is_packed && first_packed_mip_index == std::numeric_limits<size_t>::max())
        {
            first_packed_mip_index = mip.index;
        }
    }

    if (should_packed_mips_be_resident == m_packed_mips_resident)
    {
        return;
    }

    if (should_packed_mips_be_resident)
    {        
        // Allocate tiles for packed mip chain.
        m_packed_mip_tile_allocation = tile_manager.allocate_tiles(packed_tile_count);

        // Map to first packed mip index.
        tile_manager.queue_map(*this, m_packed_mip_tile_allocation, first_packed_mip_index);        
    }
    else
    {
        // Unmap the existing tiles.
        dx12_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();
        tile_manager.queue_unmap(*this, first_packed_mip_index);

        // Free the tile allocation we were using.
        tile_manager.free_tiles(m_packed_mip_tile_allocation);
        m_packed_mip_tile_allocation = {};
    }

    m_packed_mips_resident = should_packed_mips_be_resident;
}

size_t dx12_ri_texture::get_memory_usage_with_residency(size_t mip_count)
{
    size_t total_tiles = 0;
    bool added_packed_tiles = false;

    if (mip_count > m_mip_residency.size())
    {
        mip_count = m_mip_residency.size();
    }

    for (size_t i = m_mip_residency.size() - mip_count; i < m_mip_residency.size(); i++)
    {
        mip_residency& mip = m_mip_residency[i];
        if (!mip.is_packed || !added_packed_tiles)
        {
            total_tiles += mip.tile_size.NumTiles;

            if (mip.is_packed)
            {
                added_packed_tiles = true;
            }
        }
    }

    return total_tiles * D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
}

void dx12_ri_texture::make_mip_resident(size_t mip_index, const std::span<uint8_t>& linear_data)
{
    db_assert(m_create_params.is_partially_resident);

    mip_residency* residency = &m_mip_residency[mip_index];

    dx12_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();
    dx12_ri_upload_manager& upload_manager = m_renderer.get_upload_manager();

    // Only allocate tiles if none-resident.
    if (!residency->is_resident)
    {
        residency->is_resident = true;

        if (residency->is_packed)
        {
            update_packed_mip_chain_residency();
        }
        else
        {
            // Allocate tiles for this mip.
            residency->tile_allocation = tile_manager.allocate_tiles(residency->tile_size.NumTiles);

            // Map the new tiles to the textures resource.            
            tile_manager.queue_map(*this, residency->tile_allocation, residency->index);
        }
    }

    // Queue an upload for the tiles new data.
    if (!linear_data.empty())
    {
        upload_manager.upload(*this, 0, mip_index, linear_data);
    }

    // Recreate the texture views, they need to be modified to point to the correct max mip level.
    // Don't need to do this if views haven't been created yet (doing initial residency changes)
    if (m_main_srv.is_valid())
    {
        recreate_views();
    }

    // Update memory allocation.
    memory_scope mem_scope(m_memory_type, string_hash::empty, string_hash(m_debug_name));
    m_memory_allocation_info = mem_scope.record_alloc(calculate_resident_mip_used_bytes());
}

void dx12_ri_texture::make_mip_non_resident(size_t mip_index)
{
    db_assert(m_create_params.is_partially_resident);

    mip_residency& residency = m_mip_residency[mip_index];

    // Don't do anything if already non-resident.
    if (!residency.is_resident)
    {
        return;
    }
    
    residency.is_resident = false;

    if (residency.is_packed)
    {
        update_packed_mip_chain_residency();
    }
    else
    {
        // Unmap the existing tiles.
        dx12_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();
        tile_manager.queue_unmap(*this, mip_index);

        // Free the tile allocation we were using.
        tile_manager.free_tiles(residency.tile_allocation);
        residency.tile_allocation = {};
    }

    // Recreate the texture views, they need to be modified to point to the correct max mip level.
    // Don't need to do this if views haven't been created yet (doing initial residency changes)
    if (m_main_srv.is_valid())
    {
        recreate_views();
    }

    // Update memory allocation.
    memory_scope mem_scope(m_memory_type, string_hash::empty, string_hash(m_debug_name));
    m_memory_allocation_info = mem_scope.record_alloc(calculate_resident_mip_used_bytes());
}

void dx12_ri_texture::calculate_dropped_mips()
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

    // Clamp resident mips based on what mips have been entirely dropped.
    m_create_params.resident_mips = std::min(m_create_params.resident_mips, m_create_params.mip_levels);
}

void dx12_ri_texture::calculate_formats()
{
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

    UINT most_detailed_mip = (UINT)get_max_resident_mip();

    switch (m_create_params.dimensions)
    {
    case ri_texture_dimension::texture_1d:
        {
            db_assert(!m_create_params.allow_unordered_access);
            db_assert(!m_create_params.allow_individual_image_access);

            m_srv_table = ri_descriptor_table::texture_1d;

            m_main_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            m_main_srv_view_desc.Texture1D.MipLevels = static_cast<UINT16>(mip_levels - most_detailed_mip);
            m_main_srv_view_desc.Texture1D.MostDetailedMip = most_detailed_mip;
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
                rtv_template_desc.Texture1D.MipSlice = static_cast<UINT>(mip);

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
            m_main_srv_view_desc.Texture2D.MipLevels = static_cast<UINT16>(mip_levels - most_detailed_mip);
            m_main_srv_view_desc.Texture2D.MostDetailedMip = most_detailed_mip;
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
                rtv_template_desc.Texture2D.MipSlice = static_cast<UINT>(mip);

                m_rtv_view_descs[mip].push_back(rtv_template_desc);
                m_rtvs[mip].resize(1);
            }

            m_uav_view_descs.resize(mip_levels);
            m_uavs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                uav_template_desc.Texture2D.MipSlice = static_cast<UINT>(mip);

                m_uav_view_descs[mip].push_back(uav_template_desc);
                m_uavs[mip].resize(1);
            }

            m_srv_view_descs.resize(mip_levels);
            m_srvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc = m_main_srv_view_desc;
                desc.Texture2D.MipLevels = 1;
                desc.Texture2D.MostDetailedMip = static_cast<UINT>(mip);

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
            m_main_srv_view_desc.Texture3D.MipLevels = static_cast<UINT16>(mip_levels - most_detailed_mip);
            m_main_srv_view_desc.Texture3D.MostDetailedMip = most_detailed_mip;
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
                dsv_template_desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(i);
                m_dsv_view_descs.push_back(dsv_template_desc);
            }

            m_rtv_view_descs.resize(mip_levels);
            m_rtvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                m_rtvs[mip].resize(m_create_params.depth);
                for (size_t i = 0; i < m_create_params.depth; i++)
                {
                    rtv_template_desc.Texture2DArray.PlaneSlice = static_cast<UINT>(i);
                    rtv_template_desc.Texture2DArray.MipSlice = static_cast<UINT>(mip);
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
                    uav_template_desc.Texture2DArray.PlaneSlice = static_cast<UINT>(i);
                    uav_template_desc.Texture2DArray.MipSlice = static_cast<UINT>(mip);
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
                    desc.Texture2DArray.MostDetailedMip = static_cast<UINT>(mip);
                    desc.Texture2DArray.MipLevels = 1;
                    desc.Texture2DArray.PlaneSlice = static_cast<UINT>(i);

                    rtv_template_desc.Texture2DArray.PlaneSlice = static_cast<UINT>(i);
                    rtv_template_desc.Texture2DArray.MipSlice = static_cast<UINT>(mip);
                    m_srv_view_descs[mip].push_back(desc);
                }
            }

            break;
        }
    case ri_texture_dimension::texture_cube:
        {
            m_srv_table = ri_descriptor_table::texture_cube;

            m_main_srv_view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            m_main_srv_view_desc.TextureCube.MipLevels = static_cast<UINT16>(mip_levels - most_detailed_mip);
            m_main_srv_view_desc.TextureCube.MostDetailedMip = most_detailed_mip;
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
                dsv_template_desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(i);
                m_dsv_view_descs.push_back(dsv_template_desc);
            }

            m_rtv_view_descs.resize(mip_levels);
            m_rtvs.resize(mip_levels);

            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                m_rtvs[mip].resize(m_create_params.depth);
                for (size_t i = 0; i < m_create_params.depth; i++)
                {
                    rtv_template_desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(i);
                    rtv_template_desc.Texture2DArray.MipSlice = static_cast<UINT>(mip);
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
                    uav_template_desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(i);
                    uav_template_desc.Texture2DArray.MipSlice = static_cast<UINT>(mip);
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
                    desc.Texture2DArray.MostDetailedMip = static_cast<UINT>(mip);
                    desc.Texture2DArray.MipLevels = 1;
                    desc.Texture2DArray.PlaneSlice = 0;
                    desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(i);
                    desc.Texture2DArray.ArraySize = 1;

                    rtv_template_desc.Texture2DArray.PlaneSlice = static_cast<UINT>(i);
                    rtv_template_desc.Texture2DArray.MipSlice = static_cast<UINT>(mip);
                    m_srv_view_descs[mip].push_back(desc);
                }
            }

            break;
        }
    }   
}

void dx12_ri_texture::recreate_views()
{
    calculate_formats();
    free_views();
    create_views();
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

    // Notify any param blocks that reference us that they need to update their references.
    {
        std::scoped_lock lock(m_reference_mutex);

        for (dx12_ri_param_block* ref : m_referencing_param_blocks)
        {
            ref->referenced_texture_modified(this);
        }
    }
}

void dx12_ri_texture::free_views()
{
    m_renderer.defer_delete([renderer = &m_renderer, srv_table = m_srv_table, main_srv = m_main_srv, srvs = m_srvs, rtvs = m_rtvs, uavs = m_uavs, dsvs = m_dsvs]()
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
    });
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

const char* dx12_ri_texture::get_debug_name()
{
    return m_debug_name.c_str();
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

bool dx12_ri_texture::is_partially_resident() const
{
    return m_create_params.is_partially_resident;
}

size_t dx12_ri_texture::get_resident_mips()
{
    for (size_t i = 0; m_mip_residency.size(); i++)
    {
        size_t mip_index = m_mip_residency.size() - (i + 1);
        if (!m_mip_residency[mip_index].is_resident)
        {
            return i;
        }
    }
    return m_mip_residency.size();
}

bool dx12_ri_texture::is_mip_resident(size_t mip_index)
{
    return m_mip_residency[mip_index].is_resident;
}

void dx12_ri_texture::get_mip_source_data_range(size_t mip_index, size_t& offset, size_t& size)
{
    calculate_linear_data_mip_range(0, mip_index, offset, size);
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

    recreate_views();
}

}; // namespace ws
