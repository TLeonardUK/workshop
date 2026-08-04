// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/file.h"
#include "workshop.core.win32/utils/windows_headers.h"

namespace ws {

std::filesystem::path get_local_appdata_directory()
{
    return "~/.local/workshop";
}

}; // namespace workshop
