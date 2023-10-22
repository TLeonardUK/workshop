// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <string>
#include <vector>

namespace ws {

// ================================================================================================
//  Small block of information stored at the start of all compiled assets which
//  describes versioning and dependency information.
// ================================================================================================
struct compiled_asset_header
{
    // A cache key made up of both the assets cache key with all dependencies compiled.
    // If we call get_cache_key and include all the dependencies below the same result
    // should be produced in the asset is in-date.
    std::string compiled_hash;

    // ID describing the type of asset in the compiled data.
    std::string type;

    // Version number of the compiled asset format, different version
    // number formats are used for different asset types.
    size_t version;

    // Path to all other assets that contributed to the compiled data for
    // this asset. eg, include files, source files, etc. Not including
    // the source yaml file.
    std::vector<std::string> dependencies;

public:

    // Records a file as a dependency in this assets header. Duplicate
    // entries will be ignored.
    void add_dependency(const char* file);

};

// ================================================================================================
//  The base class for all asset types.
// ================================================================================================
class asset 
{
public:

    // Description of asset as loaded from compiled asset file.
    compiled_asset_header header;

    // Name of this asset, should generally point to the file
    // this asset was created from.
    std::string name;

public:
    virtual ~asset() {};

protected:
    friend class asset_manager;

    // Called after an assets data is loaded, any assets requested in this 
    // function will be considered dependencies of this asset, and this asset will not
    // be considered loaded until all the dependencies (and their dependencies)
    // have finished loading.
    virtual bool load_dependencies() { return true; };

    // Called after an asset and all its dependencies have been loaded. 
    // Can be used to do any
    // required post-processing, such as creating rendering resources, etc.
    // This will be called from a worker thread.
    // 
    // post_load is (currently) serialized, so avoid doing complex
    // logic in it, consider doing it in load_dependencies instead.
    virtual bool post_load() { return true; };

};

}; // namespace workshop
