// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/assets/texture/texture_loader.h"
#include "workshop.renderer/assets/texture/texture.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.core/filesystem/file.h"
#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"
#include "workshop.core/drawing/pixmap.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_shader_compiler.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "texture";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

// Bump if compiled format ever changes.
constexpr size_t k_asset_compiled_version = 2;

};

texture_loader::texture_loader(ri_interface& instance, renderer& renderer)
    : m_ri_interface(instance)
    , m_renderer(renderer)
{
}

const std::type_info& texture_loader::get_type()
{
    return typeid(texture);
}

asset* texture_loader::get_default_asset()
{
    return nullptr;
}

asset* texture_loader::load(const char* path)
{
    texture* asset = new texture(m_ri_interface, m_renderer);
    if (!serialize(path, *asset, false))
    {
        delete asset;
        return nullptr;
    }
    return asset;
}

void texture_loader::unload(asset* instance)
{
    delete instance;
}

bool texture_loader::save(const char* path, texture& asset)
{
    return serialize(path, asset, true);
}

bool texture_loader::serialize(const char* path, texture& asset, bool isSaving)
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

    return true;
}

bool texture_loader::load_face(const char* path, const char* face_path, texture& asset)
{
    // Load the face from.
    texture::face& face = asset.faces.emplace_back();
    face.file = face_path;
    face.pixmap = pixmap::load(face_path);

    if (!face.pixmap)
    {
        db_error(asset, "[%s] Failed to load pixmap from referenced file: %s", path, face_path);
        return false;
    }

    return true;
}

bool texture_loader::parse_faces(const char* path, YAML::Node& node, texture& asset)
{
    YAML::Node this_node = node["faces"];

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Sequence)
    {
        db_error(asset, "[%s] faces node is invalid data type.", path);
        return false;
    }

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        if (!iter->IsScalar())
        {
            db_error(asset, "[%s] face value was not scalar value.", path);
            return false;
        }
        else
        {
            std::string value = iter->as<std::string>();

            asset.header.add_dependency(value.c_str());

            if (!load_face(path, value.c_str(), asset))
            {
                return false;
            }
        }
    }

    return true;
}

bool texture_loader::parse_file(const char* path, texture& asset)
{
    db_verbose(asset, "[%s] Parsing file", path);

    YAML::Node node;
    if (!load_asset_descriptor(path, node, k_asset_descriptor_type, k_asset_descriptor_minimum_version, k_asset_descriptor_current_version))
    {
        return false;
    }

    if (!parse_faces(path, node, asset))
    {
        return false;
    }

    return true;
}

bool texture_loader::compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config)
{
    texture asset(m_ri_interface, m_renderer);

    // Parse the source YAML file that defines the shader.
    if (!parse_file(input_path, asset))
    {
        return false;
    }

    // TODO: Convert to requested format / do post-processing

    // TODO: Generate platform-specific raw data.

    // TODO: Store the raw data and clear out all the face pixmaps.

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

size_t texture_loader::get_compiled_version()
{
    return k_asset_compiled_version;
}

}; // namespace ws

