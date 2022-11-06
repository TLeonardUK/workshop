// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.engine/assets/asset_loader.h"
#include "workshop.engine/assets/types/shader/shader.h"

namespace ws {

class engine;
class asset;
class shader;
class render_interface;

// ================================================================================================
//  Loads shaders files.
// 
//  Shader files contain a description of the param blocks, render state, techniques
//  and other associated rendering data required to use a shader as part of
//  a render pass. It is not just a shader on its own.
// ================================================================================================
class shader_loader : public asset_loader
{
public:
    shader_loader(engine& instance);

    virtual const std::type_info& get_type() override;
    virtual asset* get_default_asset() override;
    virtual asset* load(const char* path) override;
    virtual void unload(asset* instance) override;
    virtual bool compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config) override;
    virtual size_t get_compiled_version() override;

private:
    bool serialize(const char* path, shader& asset, bool isSaving);

    std::string create_autogenerated_stub(shader::technique& technique, shader& asset);

    bool save(const char* path, shader& asset);

    bool compile_technique(const char* path, shader::technique& technique, shader& asset, platform_type asset_platform, config_type asset_config);

    bool parse_imports(const char* path, YAML::Node& node, shader& asset);

    bool parse_param_blocks(const char* path, YAML::Node& node, shader& asset);
    bool parse_param_block(const char* path, const char* name, YAML::Node& node, shader& asset);

    bool parse_render_states(const char* path, YAML::Node& node, shader& asset);
    bool parse_render_state(const char* path, const char* name, YAML::Node& node, shader& asset);

    bool parse_variations(const char* path, YAML::Node& node, shader& asset);
    bool parse_variation(const char* path, const char* name, YAML::Node& node, std::vector<shader::variation>& variations);

    bool parse_vertex_layouts(const char* path, YAML::Node& node, shader& asset);
    bool parse_vertex_layout(const char* path, const char* name, YAML::Node& node, shader& asset);

    bool parse_techniques(const char* path, YAML::Node& node, shader& asset);
    bool parse_technique(const char* path, const char* name, YAML::Node& node, shader& asset);

    bool parse_effects(const char* path, YAML::Node& node, shader& asset);
    bool parse_effect(const char* path, const char* name, YAML::Node& node, shader& asset);

    bool parse_file(const char* path, shader& asset);

private:
    engine& m_engine;

};

}; // namespace workshop
