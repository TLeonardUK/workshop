// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset_loader.h"
#include "workshop.renderer/assets/material/material.h"
#include "workshop.engine/ecs/object.h"

namespace ws {

class asset;
class scene;
class engine;
class component;
class reflect_class;
class reflect_field;

// ================================================================================================
//  Loads scene files.
// ================================================================================================
class scene_loader : public asset_loader
{
public:
    scene_loader(asset_manager& ass_manager, engine* engine);

    virtual const std::type_info& get_type() override;
    virtual const char* get_descriptor_type() override;
    virtual asset* get_default_asset() override;
    virtual asset* load(const char* path) override;
    virtual void unload(asset* instance) override;
    virtual bool compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config, asset_flags flags) override;
    virtual bool can_hot_reload() override { return false; };
    virtual size_t get_compiled_version() override;
    virtual bool save_uncompiled(const char* path, asset& instance) override;

private:
    struct component_info
    {
        std::string type_name;
        reflect_class* reflect_type;
        std::unique_ptr<component> component;
    };

    struct object_info
    {
        size_t handle;
        std::vector<component_info> components;
    };

    bool serialize(const char* path, scene& asset, bool isSaving);    
    bool save(const char* path, scene& asset);

    bool parse_objects(const char* path, YAML::Node& node, scene& asset, std::vector<object_info>& objects);
    bool parse_components(const char* path, YAML::Node& node, scene& asset, object_info& obj);
    bool parse_fields(const char* path, YAML::Node& node, scene& asset, component_info& comp);
    bool parse_file(const char* path, scene& asset);

private:        
    asset_manager& m_asset_manager;
    engine* m_engine;

};

}; // namespace workshop
