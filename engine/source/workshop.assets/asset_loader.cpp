// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.assets/asset_loader.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.assets/asset.h"

#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include "workshop.core/containers/string.h"

namespace ws {

template<>
inline void stream_serialize(stream& out, compiled_asset_header& header)
{
    stream_serialize(out, header.compiled_hash);
    stream_serialize(out, header.type);
    stream_serialize(out, header.version);
    stream_serialize_list(out, header.dependencies);
}

bool asset_loader::load_asset_descriptor(const char* path, YAML::Node& node, const char* expected_type, size_t min_version, size_t max_version)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, false);
    if (!stream)
    {
        db_error(asset, "[%s] Failed to open stream to asset.", path);
        return false;
    }

    std::string contents = stream->read_all_string();

    try
    {
        node = YAML::Load(contents);

        YAML::Node type_node = node["type"];
        YAML::Node version_node = node["version"];

        if (!type_node.IsDefined())
        {
            throw std::exception("type node is not defined.");
        }
        if (type_node.Type() != YAML::NodeType::Scalar)
        {
            throw std::exception("type node is the wrong type, expected a string.");
        }

        if (!version_node.IsDefined())
        {
            throw std::exception("version node is not defined.");
        }
        if (version_node.Type() != YAML::NodeType::Scalar)
        {
            throw std::exception("version node is the wrong type, expected a string.");
        }

        std::string type = type_node.as<std::string>();
        size_t version = version_node.as<size_t>();

        if (type != expected_type)
        {
            throw std::exception(string_format("Type '%s' is not of expected type '%s'.", type.c_str(), expected_type).c_str());
        }

        if (version < min_version)
        {
            throw std::exception(string_format("Version '%zi' is older than the minimum supported '%zi'.", version, min_version).c_str());
        }
        if (version > max_version)
        {
            throw std::exception(string_format("Version '%zi' is newer than the maximum supported '%zi'.", version, min_version).c_str());
        }
    }
    catch (YAML::ParserException& exception)
    {
        db_error(asset, "[%s] Error parsing asset file: %s", path, exception.msg.c_str());
        return false;
    }
    catch (std::exception& exception)
    {
        db_error(asset, "[%s] Error loading asset file: %s", path, exception.what());
        return false;
    }

    return true;
}

bool asset_loader::serialize_header(stream& out, compiled_asset_header& header, const char* path)
{
    compiled_asset_header tmp = header;
    stream_serialize(out, tmp);

    // If reading, validate the read data.
    if (!out.can_write())
    {
        if (tmp.type != header.type)
        {
            db_error(asset, "[%s] Asset descriptor type is incorrect, got '%s' expected '%s'.", path, tmp.type.c_str(), header.type.c_str());
            return false;
        }

        if (tmp.version != header.version)
        {
            db_error(asset, "[%s] Compiled asset version is incorrect, got '%zi' expected '%zi'.", path, tmp.version, header.version);
            return false;
        }
    }

    header = tmp;

    return true;
}

bool asset_loader::get_cache_key(const char* path, platform_type asset_platform, config_type asset_config, asset_cache_key& key, const std::vector<std::string>& dependencies)
{
    key.platform = asset_platform;
    key.config = asset_config;
    key.source.path = path;
    if (!virtual_file_system::get().modified_time(path, key.source.modified_time))
    {
        db_error(asset, "[%s] Could not get modification time of source file.", path);
        return false;
    }
    key.version = get_compiled_version();

    for (const std::string& dep : dependencies)
    {
        asset_cache_key_file& file = key.dependencies.emplace_back();
        file.path = dep;
        if (!virtual_file_system::get().modified_time(dep.c_str(), file.modified_time))
        {
            db_error(asset, "[%s] Could not get modification time of dependent file: %s", path, dep.c_str());
            return false;
        }
    }

    return true;
}

bool asset_loader::load_header(const char* path, compiled_asset_header& header)
{
    std::unique_ptr<stream> stream = virtual_file_system::get().open(path, false);
    if (!stream)
    {
        db_error(asset, "[%s] Failed to open stream to asset.", path);
        return false;
    }

    stream_serialize(*stream, header);

    return true;
}

}; // namespace ws
