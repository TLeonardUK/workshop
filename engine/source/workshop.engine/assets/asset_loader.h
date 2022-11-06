// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/hashing/guid.h"
#include "workshop.core/platform/platform.h"

#include "thirdparty/yamlcpp/include/yaml-cpp/yaml.h"

#include <typeinfo>

namespace ws {

class asset;
struct asset_cache_key;
class compiled_asset_header;
class stream;

// ================================================================================================
//  The base class for a loader of a given asset type.
// 
//  When an asset is loaded the local data cache is first examined for a compiled version of 
//  the asset, indexed by the value returned from get_cache_key(). 
// 
//  If no compiled version is available, then the asset is then compiled via compile().
// 
//  When a compiled version is available then load() is called with path to the compiled
//  asset.
// 
//  When a loaded asset is no longer required unload() is called before its disposed of.
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

    // Offline compiles an asset from the source data at the given path to an 
    // optimal binary file format. 
    // The resulting data will be stored and used for all future loads.
    // Returns true on success.
    virtual bool compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config) = 0;

    // Gets the current version of the compiled asset format.
    virtual size_t get_compiled_version() = 0;

public:

    // Tries to calculate the cache key used for a given asset.
    // Returns true on success, can fail if original asset file is not readable.
    bool get_cache_key(const char* path, platform_type asset_platform, config_type asset_config, asset_cache_key& key, const std::vector<std::string>& dependencies);

    // Loads the asset header only from the given file, this can be used
    // to determine if an asset needs to be recompiled.
    bool load_header(const char* path, compiled_asset_header& header);

protected:

    // Helper function, reads the YAML asset descriptor from
    // the filesystem, does basic error processing and return it.
    bool load_asset_descriptor(const char* path, YAML::Node& node, const char* expected_type, size_t min_version, size_t max_version);

    // Serializes an asset header into or out of the given stream.
    // When reading header the values read are validated to match those in the passed in header
    // if any are abnormal (eg. version missmatch) an error is logged and it returns false.
    bool serialize_header(stream& out, compiled_asset_header& header, const char* path);

};

}; // namespace workshop
