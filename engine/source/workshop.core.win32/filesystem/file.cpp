// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/file.h"
#include "workshop.core.win32/utils/windows_headers.h"

namespace ws {

std::filesystem::path get_local_appdata_directory()
{
    TCHAR szPath[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
    {
        return szPath;
    }

    // If we don't have a legitimate folder we can write to, go for the temp folder
    // so at least we can store something.
    return std::filesystem::temp_directory_path();
}

}; // namespace workshop
