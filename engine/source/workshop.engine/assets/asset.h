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

};

}; // namespace workshop
