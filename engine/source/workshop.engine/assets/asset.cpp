// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/assets/asset.h"

namespace ws {

void compiled_asset_header::add_dependency(const char* file)
{
    if (auto iter = std::find(dependencies.begin(), dependencies.end(), file); iter == dependencies.end())
    {
        dependencies.push_back(file);
    }
}

}; // namespace ws
