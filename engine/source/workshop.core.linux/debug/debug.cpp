// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/debug.h"
#include "workshop.core/containers/string.h"

#include <array>
#include <sys/prctl.h>
#include <execinfo.h>

// If se async writing will happen on a background thread to avoid spikes when writing to output.
#define USE_ASYNC_CONSOLE_LOGGING 1

namespace {

const std::array<int, static_cast<int>(ws::console_color::count)> console_color_codes = {
    0,  // unset
    31, // red
    33, // yellow
    32, // green
    90, // grey
    37, // white
};

};

namespace ws {

void db_set_thread_name(const std::string& name)
{
    prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
}

void db_break()
{
#ifndef WS_RELEASE
    __builtin_trap();
#endif
}

void db_terminate()
{
    std::terminate();
}

void db_console_write(const char* text, console_color color)
{
    printf("\003[%im%s", console_color_codes[(int)color], text);
}

result<void> db_load_symbols()
{
    db_verbose(core, "Loading symbols.");
    return true;
}

result<void> db_unload_symbols()
{
    db_verbose(core, "Unloading symbols.");
    return true;
}

std::unique_ptr<db_callstack> db_capture_callstack(size_t frame_offset, size_t frame_count)
{
    std::unique_ptr<db_callstack> result = std::make_unique<db_callstack>();

    void* buffer[256];

    // linux-todo: backtrace + backtrace_symbols
    //int ret = backtrace(, buffer, sizeof(buffer));

    return std::move(result);
}

void db_move_console(size_t x, size_t y, size_t width, size_t height)
{
    // Not supported.
}

}; // namespace workshop
