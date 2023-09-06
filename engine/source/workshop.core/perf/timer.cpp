// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/perf/timer.h"
#include "workshop.core/debug/debug.h"

namespace ws {

void timer::start()
{
    m_start_time = std::chrono::high_resolution_clock::now();
    m_started = true;
}

void timer::stop()
{
    db_assert(m_started);

    auto current_time = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::chrono::seconds::period>(current_time - m_start_time).count();
    m_elapsed_seconds += elapsed;
    m_started = false;
}

void timer::reset()
{
    m_elapsed_seconds = 0.0;
    m_started = false;
}

double timer::get_elapsed_ms()
{
    return get_elapsed_seconds() * 1000.0f;
}

double timer::get_elapsed_seconds()
{
    if (m_started)
    {
        auto current_time = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double, std::chrono::seconds::period>(current_time - m_start_time).count();
        return elapsed;
    }
    else
    {
        return m_elapsed_seconds;
    }
}


}; // namespace ws
