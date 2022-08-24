// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/filesystem/stream.h"

namespace ws {

std::string stream::read_all_string()
{
    std::string result;
    result.resize(length());
    read(result.data(), result.size());
    return std::move(result);
}

}; // namespace workshop
