// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/geometry/geometry_assimp_loader.h"
#include "workshop.core/geometry/geometry.h"
#include "workshop.core/debug/log.h"

#include "thirdparty/assimp/include/assimp/Importer.hpp"
#include "thirdparty/assimp/include/assimp/scene.h"
#include "thirdparty/assimp/include/assimp/postprocess.h"

namespace ws {

std::unique_ptr<geometry> geometry_assimp_loader::load(const std::vector<char>& buffer)
{
    std::unique_ptr<geometry> result = std::make_unique<geometry>();

    return result;
}

}; // namespace workshop
