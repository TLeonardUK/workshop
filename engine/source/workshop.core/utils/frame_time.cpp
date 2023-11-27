// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/utils/time.h"

#include <algorithm>

namespace ws {

frame_time::frame_time()
{
    m_last_frame_time = get_seconds();
}

void frame_time::step()
{
    double current_time = get_seconds();
    double elapsed = current_time - m_last_frame_time;
    m_last_frame_time = (float)current_time;

    frame_count++;
    elapsed_seconds = elapsed_seconds + (float)elapsed;
    delta_seconds = (float)std::min(k_max_step_delta, elapsed);
}

}; // namespace ws
