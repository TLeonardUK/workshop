// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_window.h"
#include "workshop.core/debug/log_handler.h"

namespace ws {

class editor_log_window;

// ================================================================================================
//  Log handler that writes messages to the editor_log_window
// ================================================================================================
struct log_handler_window
    : public log_handler
{
public:    
    log_handler_window(editor_log_window* window);

    virtual void write(log_level level, const std::string& message) override;   

private:
    editor_log_window* m_window;

};

// ================================================================================================
//  Window that shows the logging output of the game.
// ================================================================================================
class editor_log_window 
    : public editor_window
{
public:
    editor_log_window();

    virtual void draw() override;
    virtual const char* get_window_id() override;
    virtual editor_window_layout get_layout() override;

    void add_log(log_level level, const std::string& message);

private:
    struct log_entry
    {
        log_level level;
        std::vector<std::string> fragments;
    };

    static inline constexpr size_t k_max_log_entries = 1000;
    static inline constexpr size_t k_log_columns = 5;

    std::unique_ptr<log_handler_window> m_handler;
    
    std::mutex m_logs_mutex;
    std::vector<log_entry> m_logs;

};

}; // namespace ws
