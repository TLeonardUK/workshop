// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <functional>
#include <algorithm>
#include <memory>
#include <unordered_set>

namespace ws {

// Main entry point of workshop, platform specific entry point should invoke this.
int entry_point(int argc, char* argv[]);

}; // namespace workshop
