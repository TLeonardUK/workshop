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
#include "workshop.core/geometry/geometry.h"

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_shader_compiler.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "model";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

// Bump if compiled format ever changes.
constexpr size_t k_asset_compiled_version = 12;

};

template<>
inline void stream_serialize(stream& out, model::material_info& mat)
{
    stream_serialize(out, mat.name);
    stream_serialize(out, mat.file);
    stream_serialize_list(out, mat.indices);
}

template<>
inline void stream_serialize(stream& out, geometry& geo)
{
    std::vector<geometry_vertex_stream>& streams = geo.get_vertex_streams();

    stream_serialize_list(out, streams, [&out](geometry_vertex_stream& stream) {
        stream_serialize(out, stream.name);
        stream_serialize_enum(out, stream.type);
        stream_serialize(out, stream.element_size);
        stream_serialize_list(out, stream.data);
    });
}

model_loader::model_loader(ri_interface& instance, renderer& renderer, asset_manager& ass_manager)
    : m_ri_interface(instance)
    , m_renderer(renderer)
    , m_asset_manager(ass_manager)
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
    model* asset = new model(m_ri_interface, m_renderer, m_asset_manager);
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

    if (!isSaving)
    {
        asset.geometry = std::make_unique<geometry>();
    }

    stream_serialize_list(*stream, asset.materials);
    stream_serialize(*stream, *asset.geometry);

    return true;
}

bool model_loader::parse_properties(const char* path, YAML::Node& node, model& asset)
{
    std::string source;

    if (!parse_property(path, "source", node["source"], source, true))
    {
        return false;
    }

    asset.geometry = geometry::load(source.c_str());
    if (!asset.geometry)
    {
        db_error(asset, "[%s] failed to load geometry from: %s", path, source.c_str());
        return false;
    }

    return true;
}

bool model_loader::parse_materials(const char* path, YAML::Node& node, model& asset)
{
    YAML::Node this_node = node["materials"];

    if (!this_node.IsDefined())
    {
        return true;
    }

    if (this_node.Type() != YAML::NodeType::Map)
    {
        db_error(asset, "[%s] materials node is invalid data type.", path);
        return false;
    }

    for (auto iter = this_node.begin(); iter != this_node.end(); iter++)
    {
        if (!iter->second.IsScalar())
        {
            db_error(asset, "[%s] material value was not scalar value.", path);
            return false;
        }
        else
        {
            std::string name = iter->first.as<std::string>();
            std::string value = iter->second.as<std::string>();

            // Only add the material if the geometry actual uses it.
            geometry_material* geo_mat = asset.geometry->get_material(name.c_str());
            if (geo_mat != nullptr)
            {
                // Note: Don't add as dependency. We don't want to trigger a rebuild of the
                //       model just because a material has changed.
                //asset.header.add_dependency(value.c_str());

                model::material_info& mat = asset.materials.emplace_back();
                mat.name = name;
                mat.file = value;    
                mat.indices = geo_mat->indices;
            }
        }
    }

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

    if (!parse_properties(path, node, asset))
    {
        return false;
    }

    if (!parse_materials(path, node, asset))
    {
        return false;
    }

    return true;
}

bool model_loader::compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config, asset_flags flags)
{
    model asset(m_ri_interface, m_renderer, m_asset_manager);

    // Parse the source YAML file that defines the shader.
    if (!parse_file(input_path, asset))
    {
        return false;
    }

    // Construct the asset header.
    asset_cache_key compiled_key;
    if (!get_cache_key(input_path, asset_platform, asset_config, flags, compiled_key, asset.header.dependencies))
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

