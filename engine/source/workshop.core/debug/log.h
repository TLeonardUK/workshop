// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/log_handler.h"

// ================================================================================================
//  These all write various different levels of logs to whatever log handlers are registered.
// ================================================================================================
#define db_verbose(Source, Format, ...)  ws::log_handler::static_write(ws::log_level::verbose,      ws::log_source::Source, Format, __VA_ARGS__);
#define db_log(Source, Format, ...)      ws::log_handler::static_write(ws::log_level::log,          ws::log_source::Source, Format, __VA_ARGS__);
#define db_success(Source, Format, ...)  ws::log_handler::static_write(ws::log_level::success,      ws::log_source::Source, Format, __VA_ARGS__);
#define db_warning(Source, Format, ...)  ws::log_handler::static_write(ws::log_level::warning,      ws::log_source::Source, Format, __VA_ARGS__);
#define db_error(Source, Format, ...)    ws::log_handler::static_write(ws::log_level::error,        ws::log_source::Source, Format, __VA_ARGS__);
#define db_fatal(Source, Format, ...)    ws::log_handler::static_write(ws::log_level::fatal,        ws::log_source::Source, Format, __VA_ARGS__); db_assert_message(false, Format, __VA_ARGS__);

// Some general purpose debugging/assert macros.
#define db_assert(expr)                                                     \
    if (!(expr))                                                            \
    {                                                                       \
        ws::db_assert_failed(#expr, __FILE__, __LINE__, nullptr);           \
    }                 
#define db_assert_message(expr, msg, ...)                                   \
    if (!(expr))                                                            \
    {                                                                       \
        ws::db_assert_failed(#expr, __FILE__, __LINE__, msg, __VA_ARGS__);  \
    }             
    
#define db_flush() ws::log_handler::flush();

namespace ws {

// ================================================================================================
// Invoked when an assert fails, dumps an error message and callstack then terminates the application.
// ================================================================================================
void db_assert_failed(const char* expression, const char* file, size_t line, const char* msg_format, ...);

}; // namespace workshop
