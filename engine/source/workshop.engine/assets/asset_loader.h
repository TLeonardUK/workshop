// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

#include <typeinfo>

namespace ws {

class asset;

// ================================================================================================
//  The base class for a loader of a type of asset.
// ================================================================================================
class asset_loader
{
public:

    // Gets an asset that will be returned if a load fails.
    // If no default asset is supplied a fatal asset will trigger.
    virtual asset* get_default_asset() { return nullptr; }

    // Gets the type of class its capable of loading.
    virtual const std::type_info& get_type() = 0;

    // Loads an asset from the given path.
    virtual asset* load(const char* path) = 0;

    // Unloads an asset previous returned from load.
    virtual void unload(asset* instance) = 0;

protected:

    // Helper function, reads the YAML asset descriptor from
    // the filesystem, does basic error processing and return it.
    bool load_asset_descriptor(const char* path, YAML::Node& node, const char* expected_type, size_t min_version, size_t max_version);

};

}; // namespace workshop
