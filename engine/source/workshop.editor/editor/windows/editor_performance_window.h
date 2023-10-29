// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.editor/editor/utils/allocation_tree.h"
#include "workshop.core/debug/log_handler.h"
#include "workshop.core/statistics/statistics_manager.h"

namespace ws {

// ================================================================================================
//  Window that shows general performance statistics.
// ================================================================================================
class editor_performance_window 
    : public editor_window
{
public:
    editor_performance_window();

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

private:
    statistics_channel* m_stats_triangles_rendered;
    statistics_channel* m_stats_draw_calls;
    statistics_channel* m_stats_drawn_instances;
    statistics_channel* m_stats_culled_instances;
    statistics_channel* m_stats_frame_time_render;
    statistics_channel* m_stats_frame_time_game;
    statistics_channel* m_stats_frame_time_gpu;
    statistics_channel* m_stats_frame_time_render_wait;
    statistics_channel* m_stats_frame_time_present_wait;
    statistics_channel* m_stats_frame_rate;
    statistics_channel* m_stats_render_bytes_uploaded;

};

}; // namespace ws
