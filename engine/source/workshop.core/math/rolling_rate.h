// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/quat.h"
#include "workshop.core/utils/time.h"

namespace ws {

template <typename data_type, data_type window>
struct rolling_rate
{
public:
    
    // Gets the current average.
    data_type get()
    {
        trim();

        if (m_samples.empty())
        {
            return {};
        }

        data_type value_sum = {};
        data_type elapsed_sum = {};

        data_type min_time = std::numeric_limits<data_type>::max();
        data_type max_time = std::numeric_limits<data_type>::min();

        for (sample& sample : m_samples)
        {
            value_sum += sample.value;
            elapsed_sum += sample.elapsed_time;

            min_time = std::min(sample.time, min_time);
            max_time = std::max(sample.time, max_time);
        }

        data_type elapsed_time = max_time - min_time;

        return value_sum / elapsed_time;
    }

    // Adds a new sample to the average.
    void add(data_type value, data_type elapsed_time)
    {
        sample& sample = m_samples.emplace_back();
        sample.time = (data_type)get_seconds();
        sample.elapsed_time = elapsed_time;
        sample.value = value;
    }

private:

    // Strip samples that are outside of our window.
    void trim()
    {
        double current_time = get_seconds();
        for (auto iter = m_samples.begin(); iter != m_samples.end(); /*empty*/)
        {
            if ((current_time - iter->time) >= window)
            {
                iter = m_samples.erase(iter);
            }
            else
            {
                iter++;
            }
        }
    }

private:
    
    struct sample
    {
        data_type time;
        data_type elapsed_time;
        data_type value;
    };

    std::list<sample> m_samples;

};

}; // namespace ws