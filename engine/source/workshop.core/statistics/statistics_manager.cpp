// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/statistics/statistics_manager.h"
#include "workshop.core/utils/time.h"

#include <algorithm>

namespace ws {

statistics_manager::statistics_manager()
{
}

statistics_manager::~statistics_manager()
{
}

statistics_channel* statistics_manager::find_or_create_channel(const char* name, double max_history_seconds, statistics_commit_point commit_point)
{
    std::scoped_lock lock(m_mutex);

    for (auto& ptr : m_channels)
    {
        if (strcmp(ptr->get_name(), name) == 0)
        {
            // If other values are anything but default they override whatever other values has been set.
            // This is done to ensure areas just passing in the name to get the channel don't set these values
            // if they are the one to create it.
            if (max_history_seconds != 1.0 || commit_point != statistics_commit_point::none)
            {
                ptr->m_max_history_seconds = max_history_seconds;
                ptr->m_commit_point = commit_point;
            }

            return ptr.get();
        }
    }

    std::unique_ptr<statistics_channel> channel = std::make_unique<statistics_channel>(name, max_history_seconds, commit_point);
    statistics_channel* result = channel.get();
    m_channels.push_back(std::move(channel));

    return result;
}

void statistics_manager::commit(statistics_commit_point point)
{
    std::scoped_lock lock(m_mutex);

    for (auto& ptr : m_channels)
    {
        if (ptr->get_commit_point() == point)
        {
            ptr->commit();
        }
    }
}

statistics_channel::statistics_channel(const char* name, double max_history_seconds, statistics_commit_point commit_point)
    : m_name(name)
    , m_max_history_seconds(max_history_seconds)
    , m_commit_point(commit_point)
{
}

void statistics_channel::submit(double value)
{
    std::scoped_lock lock(m_mutex);

    if (m_commit_point == statistics_commit_point::none)
    {
        submit_internal(value);
    }
    else
    {
        m_aggregate += value;
        m_aggregate_samples++;
    }
}

void statistics_channel::submit_internal(double value)
{
    double time = get_seconds();

    m_samples.push_back({ value, time });

    double oldest_history_time = time - m_max_history_seconds;
    while (m_samples.front().time < oldest_history_time)
    {
        m_samples.erase(m_samples.begin());
    }
}

void statistics_channel::commit()
{
    std::scoped_lock lock(m_mutex);

    if (m_aggregate_samples > 0)
    {
        submit_internal(m_aggregate);
        m_aggregate = 0.0;
    }
}

void statistics_channel::clear()
{
    std::scoped_lock lock(m_mutex);

    m_samples.clear();
    m_aggregate = 0.0;
    m_aggregate_samples = 0;
}

double statistics_channel::current_value()
{
    std::scoped_lock lock(m_mutex);

    if (m_samples.empty())
    {
        return 0.0;
    }
    return m_samples.back().value;
}

const char* statistics_channel::get_name()
{
    return m_name.c_str();
}

statistics_commit_point statistics_channel::get_commit_point()
{
    return m_commit_point;
}

}; // namespace workshop
