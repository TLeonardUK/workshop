// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <memory>
#include <vector>

namespace ws {

class geometry;

// ================================================================================================
//  Responsible for loading data formats via assimp.
// ================================================================================================
class geometry_assimp_loader
{
public:

    // Attempts to load geometry in OBJ format from an in-memory buffer.
    static std::unique_ptr<geometry> load(const std::vector<char>& buffer);

};

}; // namespace workshop
