// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/file.h"

namespace ws {

std::filesystem::path get_local_appdata_directory()
{
    return "./.workshop_stub";
}

}; // namespace ws
