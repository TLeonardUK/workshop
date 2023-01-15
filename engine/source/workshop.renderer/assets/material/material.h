// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset.h"
#include "workshop.core/containers/string.h"

#include "workshop.render_interface/ri_types.h"

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/assets/texture/texture.h"

#include <array>
#include <unordered_map>

namespace ws {

class asset;
class ri_interface;
class renderer;
class asset_manager;

// ================================================================================================
//  Defines what part of the rendering pipeline this mateiral is going to be used in.
// ================================================================================================
enum class material_domain
{
    opaque,
    transparent,

    COUNT
};

inline static const char* material_domain_strings[static_cast<int>(material_domain::COUNT)] = {
    "opaque",
    "transparent"
};

DEFINE_ENUM_TO_STRING(material_domain, material_domain_strings)

// ================================================================================================
//  Material assets bind together all of the neccessary textures, samplers and properties required
//  to render something.
// ================================================================================================
class material : public asset
{
public:
    struct texture_info
    {
        std::string name;
        std::string path;

        asset_ptr<texture> texture;
    };

    struct sampler_info
    {
        std::string name;

        ri_texture_filter filter = ri_texture_filter::linear;

        ri_texture_address_mode address_mode_u = ri_texture_address_mode::clamp_to_edge;
        ri_texture_address_mode address_mode_v = ri_texture_address_mode::clamp_to_edge;
        ri_texture_address_mode address_mode_w = ri_texture_address_mode::clamp_to_edge;

        ri_texture_border_color border_color = ri_texture_border_color::opaque_black;

        float min_lod = 0.0f;
        float max_lod = -1.0f;
        float mip_lod_bias = 0.0f;

        int max_anisotropy = 0;

        std::unique_ptr<ri_sampler> ri_sampler;
    };

    struct parameter_info
    {
        std::string name;
        std::string value;
    };

public:
    material(ri_interface& ri_interface, renderer& renderer, asset_manager& ass_manager);
    virtual ~material();

public:
    material_domain domain = material_domain::opaque;

    std::vector<texture_info> textures;
    std::vector<sampler_info> samplers;
    std::vector<parameter_info> parameters;

protected:
    virtual bool post_load() override;

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;
    asset_manager& m_asset_manager;

};

}; // namespace workshop
