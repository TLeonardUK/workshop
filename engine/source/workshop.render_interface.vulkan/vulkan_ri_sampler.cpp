// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_sampler.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_types.h"

namespace ws {

vulkan_ri_sampler::vulkan_ri_sampler(vulkan_render_interface& renderer, const char* debug_name, const create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name ? debug_name : "")
    , m_create_params(params)
{
}

vulkan_ri_sampler::~vulkan_ri_sampler()
{
    m_renderer.defer_delete([renderer = &m_renderer, sampler = m_sampler, handle = m_handle]()
    {
        if (sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(renderer->get_device(), sampler, nullptr);
        }
        if (handle.is_valid)
        {
            renderer->get_descriptor_table(ri_descriptor_table::sampler).free(handle);
        }
    });
}

result<void> vulkan_ri_sampler::create_resources()
{
    color border_color = ri_to_vulkan(m_create_params.border_color);

    VkSamplerCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter = ri_to_vulkan_min_mag_filter(m_create_params.filter);
    create_info.minFilter = ri_to_vulkan_min_mag_filter(m_create_params.filter);
    create_info.mipmapMode = ri_to_vulkan_mip_filter(m_create_params.filter);
    create_info.addressModeU = ri_to_vulkan(m_create_params.address_mode_u);
    create_info.addressModeV = ri_to_vulkan(m_create_params.address_mode_v);
    create_info.addressModeW = ri_to_vulkan(m_create_params.address_mode_w);
    create_info.mipLodBias = m_create_params.mip_lod_bias;
    create_info.anisotropyEnable = ri_to_vulkan_is_anisotropic(m_create_params.filter) ? VK_TRUE : VK_FALSE;
    create_info.maxAnisotropy = static_cast<float>(m_create_params.max_anisotropy);
    create_info.compareEnable = VK_FALSE;
    create_info.compareOp = VK_COMPARE_OP_NEVER;
    create_info.minLod = m_create_params.min_lod;
    create_info.maxLod = m_create_params.max_lod;
    create_info.borderColor = (border_color.a > 0.5f)
        ? ((border_color.r > 0.5f) ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE : VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK)
        : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    create_info.unnormalizedCoordinates = VK_FALSE;

    VkResult vk_result = vkCreateSampler(m_renderer.get_device(), &create_info, nullptr, &m_sampler);
    if (!m_renderer.check_result(vk_result, "vkCreateSampler"))
    {
        return standard_errors::failed;
    }

    m_handle = m_renderer.get_descriptor_table(ri_descriptor_table::sampler).allocate();
    m_renderer.get_descriptor_table(ri_descriptor_table::sampler).write_sampler(m_handle, m_sampler);

    return true;
}

size_t vulkan_ri_sampler::get_descriptor_table_index() const
{
    return m_handle.index;
}

ri_texture_filter vulkan_ri_sampler::get_filter()
{
    return m_create_params.filter;
}

ri_texture_address_mode vulkan_ri_sampler::get_address_mode_u()
{
    return m_create_params.address_mode_u;
}

ri_texture_address_mode vulkan_ri_sampler::get_address_mode_v()
{
    return m_create_params.address_mode_v;
}

ri_texture_address_mode vulkan_ri_sampler::get_address_mode_w()
{
    return m_create_params.address_mode_w;
}

ri_texture_border_color vulkan_ri_sampler::get_border_color()
{
    return m_create_params.border_color;
}

float vulkan_ri_sampler::get_min_lod()
{
    return m_create_params.min_lod;
}

float vulkan_ri_sampler::get_max_lod()
{
    return m_create_params.max_lod;
}

float vulkan_ri_sampler::get_mip_lod_bias()
{
    return m_create_params.mip_lod_bias;
}

int vulkan_ri_sampler::get_max_anisotropy()
{
    return m_create_params.max_anisotropy;
}

}; // namespace ws
