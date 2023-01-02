// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <chrono>

namespace ws {

// ================================================================================================
//  Super simple high-resolution timer.
//  This basically provides a nicer interface to the chrono library.
// ================================================================================================
class timer
{
public:
    
    // Starts timing.
    void start();

    // Stops timing and adds the duration to the elapsed total.
    void stop();

    // Stops timing and resets the total elapsed total to 0.
    void reset();

    // Gets the number of elapsed milliseconds.
    double get_elapsed_ms();

    // Gets the number of elapsed seconds.
    double get_elapsed_seconds();

private:

    std::chrono::high_resolution_clock::time_point m_start_time;

    double m_elapsed_seconds = 0.0;
    bool m_started = false;

};

}; // namespace workshop
