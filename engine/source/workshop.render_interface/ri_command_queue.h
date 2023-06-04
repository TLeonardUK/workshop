// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/perf/profile.h"

#include "workshop.render_interface/ri_command_list.h"

namespace ws {

class ri_command_list;

// ================================================================================================
//  Represents a single queue of execution on the gpu.
// ================================================================================================
class ri_command_queue
{
public:
    virtual ~ri_command_queue() {}

    // Allocates a new command list which can be submitted to this queue.
    // This list is valid for the current frame.
    virtual ri_command_list& alloc_command_list() = 0;

    // Inserts a command list for execution on this queue.
    virtual void execute(ri_command_list& list) = 0;

    // Begins a profiling scope within the queue.
    virtual void begin_event(const color& color, const char* name, ...) = 0;

    // End a profiling scope within the queue.
    virtual void end_event() = 0;

};

// RAII gpu marker class. 
template <typename... Args>
struct ri_scoped_gpu_profile_marker
{
    ri_scoped_gpu_profile_marker(ri_command_queue& queue, const color& color, const char* format, Args... args)
        : m_queue(&queue)
    {
        m_queue->begin_event(color, format, std::forward<Args>(args)...);
    }

    ri_scoped_gpu_profile_marker(ri_command_list& list, const color& color, const char* format, Args... args)
        : m_list(&list)
    {
        m_list->begin_event(color, format, std::forward<Args>(args)...);
    }

    ~ri_scoped_gpu_profile_marker()
    {
        if (m_queue)
        {
            m_queue->end_event();
        }
        else
        {
            m_list->end_event();
        }
    }

private:
    ri_command_queue* m_queue = nullptr;
    ri_command_list* m_list = nullptr;
};

#define profile_gpu_marker_make_id_2(x) x
#define profile_gpu_marker_make_id(name, line) profile_gpu_marker_make_id_2(name)##profile_gpu_marker_make_id_2(line)
#ifdef WS_RELEASE
#define profile_gpu_marker(queue, color, name, ...)
#else
#define profile_gpu_marker(queue, color, name, ...) ri_scoped_gpu_profile_marker profile_gpu_marker_make_id(pm_gpu_, __LINE__)(queue, color, name, __VA_ARGS__)
#endif

}; // namespace ws
