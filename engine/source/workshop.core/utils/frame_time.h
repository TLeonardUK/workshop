// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <chrono>

namespace ws {

// ================================================================================================
//  Holds frame timing information used for delta-timing/general updating.
// ================================================================================================
class frame_time
{
public:

    frame_time();

    static const inline float k_max_step_delta = 1.0f / 15.0f;

    // Time between last frame and this frame in seconds. This is clamped to k_max_step_delta.
    float delta_seconds = 0.0f;

    // Number of frames that have elapsed since the engine started.
    size_t frame_count = 0;

    // The time since the engine started. This will eventually loose 
    // accuracy as play-time increases, so prefer use of delta_seconds.
    float elapsed_seconds = 0.0f;

public:

    // Called each frame to update the time for the coming frame.
    void step();

private:
    
    std::chrono::high_resolution_clock::time_point m_last_frame_time;

};

}; // namespace ws
