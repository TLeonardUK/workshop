// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/types/shader/shader_loader.h"
#include "workshop.engine/assets/types/shader/shader.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

namespace ws {

namespace {

constexpr const char* k_asset_descriptor_type = "Shader";
constexpr size_t k_asset_descriptor_minimum_version = 1;
constexpr size_t k_asset_descriptor_current_version = 1;

};

const std::type_info& shader_loader::get_type()
{
    return typeid(shader);
}

asset* shader_loader::get_default_asset()
{
    return nullptr;
}

asset* shader_loader::load(const char* path)
{
    YAML::Node node;
    if (!load_asset_descriptor(path, node, k_asset_descriptor_type, k_asset_descriptor_minimum_version, k_asset_descriptor_current_version))
    {
        return nullptr;
    }

    // TODO

    return nullptr;

}

void shader_loader::unload(asset* instance)
{
        
}

}; // namespace ws
