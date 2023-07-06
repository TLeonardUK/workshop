// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset_loader.h"
#include "workshop.renderer/assets/material/material.h"

namespace ws {

class asset;
class material;
class render_interface;
class ri_interface;
class renderer;

// ================================================================================================
//  Loads material files.
// ================================================================================================
class material_loader : public asset_loader
{
public:
    material_loader(ri_interface& instance, renderer& renderer, asset_manager& ass_manager);

    virtual const std::type_info& get_type() override;
    virtual asset* get_default_asset() override;
    virtual asset* load(const char* path) override;
    virtual void unload(asset* instance) override;
    virtual bool compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config) override;
    virtual void hot_reload(asset* instance, asset* new_instance) override;
    virtual bool can_hot_reload() override { return true; };
    virtual size_t get_compiled_version() override;

private:
    bool serialize(const char* path, material& asset, bool isSaving);

    bool parse_textures(const char* path, YAML::Node& node, material& asset);
    bool parse_samplers(const char* path, YAML::Node& node, material& asset);
    bool parse_sampler(const char* path, const char* name, YAML::Node& node, material& asset);
    bool parse_parameters(const char* path, YAML::Node& node, material& asset);

    bool save(const char* path, material& asset);

    bool parse_file(const char* path, material& asset);

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;
    asset_manager& m_asset_manager;

};

}; // namespace workshop
