// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/debug/log.h"
#include "workshop.core/utils/result.h"

namespace ws {

// Colors that can be used when emitting text to the system console.
enum class console_color
{
    unset,
    red,
    yellow,
    green,
    grey,
    white,

    count
};

// Represents a callstack captured from db_capture_callstack.
struct db_callstack
{
    struct frame
    {
        size_t address;
        std::string module;
        std::string function;
        std::string filename;
        size_t line;
    };

    std::vector<frame> frames;
};

// ================================================================================================
// Sets the thread name in the debugger. This only has an effect in visual studio.
// ================================================================================================
void db_set_thread_name(const std::string& name);

// ================================================================================================
// Manually trigger breakpoint. Does nothing in release builds.
// ================================================================================================
void db_break();

// ================================================================================================
// Hard terminates the application without running any shutdown processing. This will result
// in a failure error code being the execution result.
// ================================================================================================
void db_terminate();

// ================================================================================================
// Writes the given text to the console.
// Colors are platform dependent and are not guaranteed to be used.
// ================================================================================================
void db_console_write(const char* text, console_color color = console_color::unset);

// ================================================================================================
// Loads symbols files if they are available. The symbols will be used for resolving 
// function names and similar for functions like db_capture_callstack.
// ================================================================================================
result<void> db_load_symbols();

// ================================================================================================
// Cleans up symbols previously loaded by db_load_symbols.
// ================================================================================================
result<void> db_unload_symbols();

// ================================================================================================
// Writes the given text to the console.
// Colors are platform dependent and are not guaranteed to be used.
// ================================================================================================
std::unique_ptr<db_callstack> db_capture_callstack(size_t frame_offset = 0, size_t frame_count = INT_MAX);

}; // namespace workshop
