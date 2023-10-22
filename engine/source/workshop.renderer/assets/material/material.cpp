// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/material/material.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.renderer/render_effect_manager.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.assets/asset_manager.h"

namespace ws {

material::material(ri_interface& ri_interface, renderer& renderer, asset_manager& ass_manager)
    : m_ri_interface(ri_interface)
    , m_renderer(renderer)
    , m_asset_manager(ass_manager)
{
}

material::~material()
{
}

bool material::load_dependencies()
{
    for (sampler_info& info : samplers)
    {
        ri_sampler::create_params params;
        params.filter = info.filter;
        params.address_mode_u = info.address_mode_u;
        params.address_mode_v = info.address_mode_v;
        params.address_mode_w = info.address_mode_w;
        params.border_color = info.border_color;
        params.min_lod = info.min_lod;
        params.max_lod = info.max_lod;
        params.mip_lod_bias = info.mip_lod_bias;
        params.max_anisotropy = info.max_anisotropy;

        info.ri_sampler = m_ri_interface.create_sampler(params, info.name.c_str());
    }

    for (texture_info& info : textures)
    {
        info.texture = m_asset_manager.request_asset<texture>(info.path.c_str(), 0);
    }

    return true;
}

bool material::post_load()
{
    create_material_info_param_block();

    return true;
}

void material::create_material_info_param_block()
{
    ri_texture* default_black = m_renderer.get_default_texture(default_texture_type::black);
    ri_texture* default_white = m_renderer.get_default_texture(default_texture_type::white);
    ri_texture* default_grey = m_renderer.get_default_texture(default_texture_type::grey);
    ri_texture* default_normal = m_renderer.get_default_texture(default_texture_type::normal);
    ri_sampler* default_sampler_color = m_renderer.get_default_sampler(default_sampler_type::color);
    ri_sampler* default_sampler_normal = m_renderer.get_default_sampler(default_sampler_type::normal);

    m_material_info_param_block = m_renderer.get_param_block_manager().create_param_block("material_info");
    
    m_material_info_param_block->set("domain", (int)domain);
    
    m_material_info_param_block->set("albedo_texture", *get_texture("albedo_texture", default_black));
    m_material_info_param_block->set("opacity_texture", *get_texture("opacity_texture", default_white));
    m_material_info_param_block->set("metallic_texture", *get_texture("metallic_texture", default_black));
    m_material_info_param_block->set("roughness_texture", *get_texture("roughness_texture", default_grey));
    m_material_info_param_block->set("normal_texture", *get_texture("normal_texture", default_normal));
    m_material_info_param_block->set("skybox_texture", *get_texture("skybox_texture", default_white));

    m_material_info_param_block->set("albedo_sampler", *get_sampler("albedo_sampler", default_sampler_color));
    m_material_info_param_block->set("opacity_sampler", *get_sampler("opacity_sampler", default_sampler_color));
    m_material_info_param_block->set("metallic_sampler", *get_sampler("metallic_sampler", default_sampler_color));
    m_material_info_param_block->set("roughness_sampler", *get_sampler("roughness_sampler", default_sampler_color));
    m_material_info_param_block->set("normal_sampler", *get_sampler("normal_sampler", default_sampler_normal));
    m_material_info_param_block->set("skybox_sampler", *get_sampler("skybox_sampler", default_sampler_color));
}

ri_sampler* material::get_sampler(const char* name, ri_sampler* default_instance)
{
    for (sampler_info& info : samplers)
    {
        if (info.name == name)
        {
            return info.ri_sampler.get();
        }
    }
    return default_instance;
}

ri_texture* material::get_texture(const char* name, ri_texture* default_instance)
{
    for (texture_info& info : textures)
    {
        if (info.name == name && info.texture.is_loaded())
        {
            return info.texture->ri_instance.get();
        }
    }
    return default_instance;
}

ri_param_block* material::get_material_info_param_block()
{
    return m_material_info_param_block.get();
}

void material::swap(material* other)
{
    std::swap(domain, other->domain);
    std::swap(textures, other->textures);
    std::swap(samplers, other->samplers);
    std::swap(parameters, other->parameters);
    std::swap(m_material_info_param_block, other->m_material_info_param_block);
}

}; // namespace ws
