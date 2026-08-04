// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_sampler.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"

namespace ws {

dx12_ri_sampler::dx12_ri_sampler(dx12_render_interface& renderer, const char* debug_name, const ri_sampler::create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
{
}

dx12_ri_sampler::~dx12_ri_sampler()
{
    m_renderer.defer_delete([handle = m_handle, renderer = &m_renderer]() 
    {
        if (handle.is_valid())
        {
            renderer->get_descriptor_table(ri_descriptor_table::sampler).free(handle);
        }
    });
}

result<void> dx12_ri_sampler::create_resources()
{
    color border_color = ri_to_dx12(m_create_params.border_color);

    D3D12_SAMPLER_DESC desc;
    desc.Filter = ri_to_dx12(m_create_params.filter);
    desc.AddressU = ri_to_dx12(m_create_params.address_mode_u);
    desc.AddressV = ri_to_dx12(m_create_params.address_mode_v);
    desc.AddressW = ri_to_dx12(m_create_params.address_mode_w);
    desc.MipLODBias = m_create_params.mip_lod_bias;
    desc.MaxAnisotropy = m_create_params.max_anisotropy;
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    desc.BorderColor[0] = border_color.r;
    desc.BorderColor[1] = border_color.g;
    desc.BorderColor[2] = border_color.b;
    desc.BorderColor[3] = border_color.a;
    desc.MinLOD = m_create_params.min_lod;
    desc.MaxLOD = m_create_params.max_lod;

    m_handle = m_renderer.get_descriptor_table(ri_descriptor_table::sampler).allocate();

    m_renderer.get_device()->CreateSampler(&desc, m_handle.cpu_handle);

    return true;
}

size_t dx12_ri_sampler::get_descriptor_table_index() const
{
    return m_handle.get_table_index();
}

ri_texture_filter dx12_ri_sampler::get_filter()
{
    return m_create_params.filter;
}

ri_texture_address_mode dx12_ri_sampler::get_address_mode_u()
{
    return m_create_params.address_mode_u;
}

ri_texture_address_mode dx12_ri_sampler::get_address_mode_v()
{
    return m_create_params.address_mode_v;
}

ri_texture_address_mode dx12_ri_sampler::get_address_mode_w()
{
    return m_create_params.address_mode_w;
}

ri_texture_border_color dx12_ri_sampler::get_border_color()
{
    return m_create_params.border_color;
}

float dx12_ri_sampler::get_min_lod()
{
    return m_create_params.min_lod;
}

float dx12_ri_sampler::get_max_lod()
{
    return m_create_params.max_lod;
}

float dx12_ri_sampler::get_mip_lod_bias()
{
    return m_create_params.mip_lod_bias;
}

int dx12_ri_sampler::get_max_anisotropy()
{
    return m_create_params.max_anisotropy;
}

}; // namespace ws
