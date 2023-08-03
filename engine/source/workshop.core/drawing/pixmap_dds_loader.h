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
//  Responsible for loading DDS format data.
// ================================================================================================
class pixmap_dds_loader
{
public:

    // Attempts to load a pixmap in DDS format from an in-memory buffer.
    static std::unique_ptr<pixmap> load(const std::vector<char>& buffer);

    // Attempts to save a pixmap in DDS format to an in-memory buffer.
    static bool save(pixmap& input, std::vector<char>& buffer);

};

}; // namespace workshop
