// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/asset_loader.h"

#include "workshop.core/filesystem/stream.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include "workshop.core/containers/string.h"

namespace ws {

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

}; // namespace ws
