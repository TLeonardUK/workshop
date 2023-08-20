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

    virtual void write_raw(log_level level, log_source source, const std::string& timestamp, const std::string& message) override;

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

    void add_log(log_level level, log_source source, const std::string& timestamp, const std::string& message);

private:
    void apply_filter();

private:
    struct log_entry
    {
        log_level level;
        log_source category;
        std::string message;
        std::string timestamp;
        bool filtered_out = false;
        std::string search_key;
    };

    static inline constexpr size_t k_max_log_entries = 1000;
    static inline constexpr size_t k_log_columns = 5;

    std::unique_ptr<log_handler_window> m_handler;
    
    std::mutex m_logs_mutex;
    std::vector<log_entry> m_logs;

    log_level m_base_max_log_level;

    // 0=all, all other values = log_level
    int m_log_level = 0;

    // 0=all, all other values = log_category
    int m_log_category = 0;

    char m_filter_buffer[256] = { '\0' };

};

}; // namespace ws
