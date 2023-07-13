// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/perf/profile.h"
#include "workshop.core/filesystem/file.h"

#include "workshop.core.win32/utils/windows_headers.h"
#include "thirdparty/pix/include/pix3.h"

#include <filesystem>
#include <shlobj.h>

namespace ws {

std::string get_latest_pix_gpu_dll()
{
    // As per: https://devblogs.microsoft.com/pix/taking-a-capture/

    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

    std::filesystem::path pixInstallationPath = programFilesPath;
    pixInstallationPath /= "Microsoft PIX";

    std::string newestVersionFound;

    for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
    {
        if (directory_entry.is_directory())
        {
            if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().string())
            {
                newestVersionFound = directory_entry.path().filename().string();
            }
        }
    }

    if (newestVersionFound.empty())
    {
        db_warning(core, "No version of PIX gpu runtime found, attaching for gpu capture will not be possible.");
        return "";
    }

    return (pixInstallationPath / newestVersionFound / "WinPixGpuCapturer.dll").string();
}
    
void platform_perf_init()
{
#ifndef WS_RELEASE
    //if (ws::is_option_set("load_pix"))
    {
        std::string pix_gpu_path = get_latest_pix_gpu_dll();
        if (!pix_gpu_path.empty())
        {
            if (GetModuleHandleA("WinPixGpuCapturer.dll") == 0)
            {
                db_log(core, "Loading PIX gpu runtime from: %s", pix_gpu_path.c_str());
                LoadLibraryA(pix_gpu_path.c_str());
            }
        }
    }
#endif
}

void platform_perf_begin_marker(const color& color, const char* format, ...)
{
    uint8_t r, g, b, a;
    color.get(r, g, b, a);

    char buffer[1024];
    
    va_list list;
    va_start(list, format);

    int ret = vsnprintf(buffer, sizeof(buffer), format, list);
    int space_required = ret + 1;
    if (ret >= sizeof(buffer))
    {
        return;
    }

    PIXBeginEvent(PIX_COLOR(r, g, b), "%s", buffer);

    va_end(list);
}

void platform_perf_end_marker()
{
    PIXEndEvent();
}

void platform_perf_variable(double value, const char* format, ...)
{
    char buffer[1024];

    va_list list;
    va_start(list, format);

    int ret = vsnprintf(buffer, sizeof(buffer), format, list);
    int space_required = ret + 1;
    if (ret >= sizeof(buffer))
    {
        return;
    }

    PIXReportCounter(widen_string(buffer).c_str(), static_cast<float>(value));

    va_end(list);
}

}; // namespace workshop
