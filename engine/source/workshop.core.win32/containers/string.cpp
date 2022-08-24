// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/containers/string.h"
#include "workshop.core.win32/utils/windows_headers.h"

namespace ws {

std::string narrow_string(const std::wstring& input)
{
    std::string result;

    int result_length = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.length()), nullptr, 0, nullptr, nullptr);
    db_assert(result_length > 0);
    if (result_length <= 0)
    {
        return "";
    }

    result.resize(result_length);
    WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.length()), result.data(), static_cast<int>(result.length()), nullptr, nullptr);

    return result;
}

std::wstring widen_string(const std::string& input)
{
    std::wstring result;
    result.resize(input.size());

    int result_length = MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.length()), result.data(), static_cast<int>(result.length()));
    db_assert(result_length > 0);
    if (result_length <= 0)
    {
        return L"";
    }

    result.resize(result_length);

    return result;
}

}; // namespace workshop
