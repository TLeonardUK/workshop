// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/containers/string.h"

namespace ws {

std::string narrow_string(const std::wstring& input)
{
    return std::string(input.begin(), input.end());
}

std::wstring widen_string(const std::string& input)
{
    return std::wstring(input.begin(), input.end());
}

}; // namespace ws
