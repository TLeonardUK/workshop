// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"

namespace ws {

struct version_info
{
    int major;
    int minor;
    int revision;
    int build;

    std::string string;
    std::string changeset;
};

// ================================================================================================
//  Gets the version of the compiled application.
// ================================================================================================
version_info get_version();

}; // namespace workshop
