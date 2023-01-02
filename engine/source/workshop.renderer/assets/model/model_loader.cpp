// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/model/model_loader.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_shader_compiler.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "model";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

// Bump if compiled format ever changes.
constexpr size_t k_asset_compiled_version = 2;

};

model_loader::model_loader(ri_interface& instance, renderer& renderer)
    : m_ri_interface(instance)
    , m_renderer(renderer)
{
}

const std::type_info& model_loader::get_type()
{
    return typeid(model);
}

asset* model_loader::get_default_asset()
{
    return nullptr;
}

asset* model_loader::load(const char* path)
{
    model* asset = new model(m_ri_interface, m_renderer);
    if (!serialize(path, *asset, false))
    {
        delete asset;
        return nullptr;
    }
    return asset;
}

void model_loader::unload(asset* instance)
{
    delete instance;
}

bool model_loader::save(const char* path, model& asset)
{
    return serialize(path, asset, true);
}

bool model_loader::serialize(const char* path, model& asset, bool isSaving)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, isSaving);
    if (!stream)
    {
        db_error(asset, "[%s] Failed to open stream to save asset.", path);
        return false;
    }

    if (!isSaving)
    {
        asset.header.type = k_asset_descriptor_type;
        asset.header.version = k_asset_compiled_version;
        asset.name = path;
    }

    if (!serialize_header(*stream, asset.header, path))
    {
        return false;
    }

    // TODO: Serialize anything valid.

    return true;
}

bool model_loader::parse_file(const char* path, model& asset)
{
    db_verbose(asset, "[%s] Parsing file", path);

    YAML::Node node;
    if (!load_asset_descriptor(path, node, k_asset_descriptor_type, k_asset_descriptor_minimum_version, k_asset_descriptor_current_version))
    {
        return false;
    }
    
    // TODO: Parse

    return true;
}

bool model_loader::compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config)
{
    model asset(m_ri_interface, m_renderer);

    // Parse the source YAML file that defines the shader.
    if (!parse_file(input_path, asset))
    {
        return false;
    }

    // TODO: Do post processing.

    // Construct the asset header.
    asset_cache_key compiled_key;
    if (!get_cache_key(input_path, asset_platform, asset_config, compiled_key, asset.header.dependencies))
    {
        db_error(asset, "[%s] Failed to calculate compiled cache key.", input_path);
        return false;
    }
    asset.header.compiled_hash = compiled_key.hash();
    asset.header.type = k_asset_descriptor_type;
    asset.header.version = k_asset_compiled_version;

    // Write binary format to disk.
    if (!save(output_path, asset))
    {
        return false;
    }

    return true;
}

size_t model_loader::get_compiled_version()
{
    return k_asset_compiled_version;
}

}; // namespace ws

