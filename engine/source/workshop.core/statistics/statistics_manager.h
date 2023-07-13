// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/singleton.h"

#include <string>
#include <vector>
#include <mutex>

namespace ws {

class stream;

// ================================================================================================
//  Represents the point at which aggregated values in a channel will be committed.
// ================================================================================================
enum class statistics_commit_point
{
    // Not aggregated, samples will be submitted individually.
    none,

    // Occurs at the end of the game loop.
    end_of_game,

    // Occurs at the end of the render.
    end_of_render,
};

// ================================================================================================
//  Represents an individual type of statistics registered with the statistics_manager. 
//  This can be used for submitted or querying the current or transformed values of the stat.
// ================================================================================================
class statistics_channel
{
public:
    statistics_channel(const char* name, double max_history_seconds, statistics_commit_point commit_point);

    // Submits a new sample value to the channel.
    void submit(double value);

    // Clears any samples currently in the channel.
    void clear();

    // Gets the latest value in the channel.
    double current_value();

    // Gets the name of this channel.
    const char* get_name();

    // Gets point at which aggregated values are committed.
    statistics_commit_point get_commit_point();

private:
    friend class statistics_manager;

    // Commits the result of multiple aggregate calls.
    void commit();

    // Adds the value to the sample list.
    void submit_internal(double value);

private:
    struct sample
    {
        double value;
        double time;
    };

    std::recursive_mutex m_mutex;

    std::string m_name;

    double m_max_history_seconds;
    std::vector<sample> m_samples;

    statistics_commit_point m_commit_point;

    double m_aggregate = 0.0f;
    size_t m_aggregate_samples = 0;

};

// ================================================================================================
//  This class is responsible for taking and storing arbitrary numeric statistics reported
//  throughout the engine. These statistics can be anything from the frame rate to the number
//  of triangles rendered.
// 
//  The statistics system is thread-safe.
// ================================================================================================
class statistics_manager
    : public singleton<statistics_manager>
{
public:
    statistics_manager();
    ~statistics_manager();

    // Attempts to find a channel with the given name, or if none is found then
    // a new one is created. The channels persist until the statistics manager
    // is destroyed.
    //
    // history_seconds dictates how many seconds of historical data the channel should store
    // for transforming the data.
    //
    // commit_point determines when aggregated values will be committed. If set to none then
    // individual submits will not be aggregated.
    statistics_channel* find_or_create_channel(const char* name, double max_history_seconds = 1.0, statistics_commit_point commit_point = statistics_commit_point::none);

    // Commits all aggregate statistics waiting on the given point.
    void commit(statistics_commit_point point);

private:
    std::mutex m_mutex;
    std::vector<std::unique_ptr<statistics_channel>> m_channels;

};

}; // namespace workshop
