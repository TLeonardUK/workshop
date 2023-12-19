// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset_loader.h"
#include "workshop.renderer/assets/model/model.h"

namespace ws {

class asset;
class model;
class render_interface;
class ri_interface;
class renderer;
class asset_manager;

// ================================================================================================
//  Loads model files.
// 
//  Model files contain vertex/index data along with misc data such as
//  animations and material references.
// ================================================================================================
class model_loader : public asset_loader
{
public:
    model_loader(ri_interface& instance, renderer& renderer, asset_manager& ass_manager);

    virtual const std::type_info& get_type() override;
    virtual const char* get_descriptor_type() override;
    virtual asset* get_default_asset() override;
    virtual asset* load(const char* path) override;
    virtual void unload(asset* instance) override;
    virtual bool compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config, asset_flags flags) override;
    virtual void hot_reload(asset* instance, asset* new_instance) override;
    virtual bool can_hot_reload() override { return true; };
    virtual size_t get_compiled_version() override;

private:
    bool serialize(const char* path, model& asset, bool isSaving);

    bool save(const char* path, model& asset);

    bool parse_properties(const char* path, YAML::Node& node, model& asset);
    bool parse_materials(const char* path, YAML::Node& node, model& asset);
    bool parse_mesh_namelist(const char* path, YAML::Node& node, model& asset, const char* key, std::vector<std::string>& output);
    bool parse_blacklist(const char* path, YAML::Node& node, model& asset);
    bool parse_file(const char* path, model& asset);

    void calculate_streaming_info(model& asset);

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;
    asset_manager& m_asset_manager;

};

}; // namespace workshop
