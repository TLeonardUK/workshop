// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"

#include <memory>
#include <vector>

struct aiScene;
struct aiNode;

namespace ws {

class geometry;

// ================================================================================================
//  Responsible for loading data formats via assimp.
// ================================================================================================
class geometry_assimp_loader
{
public:

    // Attempts to load geometry in OBJ format from an in-memory buffer.
    static std::unique_ptr<geometry> load(const std::vector<char>& buffer, const char* path_hint, const vector3& scale, bool high_quality);

    // Returns true if the extension is one that this loader supports.
    static bool supports_extension(const char* extension);

};

}; // namespace workshop
