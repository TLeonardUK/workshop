// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/debug.h"

#include <cstdio>
#include <cstdlib>

namespace ws {

void db_set_thread_name(const std::string& name)
{
    // Not supported by the stub platform.
}

void db_break()
{
    db_flush();
#ifndef WS_RELEASE
    std::abort();
#endif
}

void db_terminate()
{
    db_flush();
    std::terminate();
}

void db_console_write(const char* text, console_color color)
{
    printf("%s", text);
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
    return std::make_unique<db_callstack>();
}

void db_move_console(size_t x, size_t y, size_t width, size_t height)
{
    // Not supported.
}

}; // namespace ws
