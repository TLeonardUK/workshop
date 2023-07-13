// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/utils/time.h"

#include <chrono>

namespace ws {

double get_seconds()
{
    static std::chrono::high_resolution_clock::time_point s_start_time = std::chrono::high_resolution_clock::now();

    std::chrono::high_resolution_clock::time_point current_time = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::chrono::seconds::period>(current_time - s_start_time).count();

    return elapsed;
}

}; // namespace workshop
