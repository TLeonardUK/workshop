// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/utils/frame_time.h"

namespace ws {

frame_time::frame_time()
{
    m_last_frame_time = std::chrono::high_resolution_clock::now();
}

void frame_time::step()
{
    auto current_time = std::chrono::high_resolution_clock::now();
    float elapsed = std::chrono::duration<float, std::chrono::seconds::period>(current_time - m_last_frame_time).count();
    m_last_frame_time = current_time;

    frame_count++;
    elapsed_seconds += elapsed;
    delta_seconds = std::min(k_max_step_delta, elapsed);
}

}; // namespace ws
