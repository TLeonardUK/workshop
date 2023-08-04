// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <memory>
#include <vector>

namespace ws {

class pixmap;

// ================================================================================================
//  Responsible for loading PNG format data.
// ================================================================================================
class pixmap_png_loader
{
public:

    // Attempts to load a pixmap in PNG format from an in-memory buffer.
    static std::vector<std::unique_ptr<pixmap>> load(const std::vector<char>& buffer);

    // Attempts to save a pixmap in PNG format to an in-memory buffer.
    static bool save(pixmap& input, std::vector<char>& buffer);

};

}; // namespace workshop
