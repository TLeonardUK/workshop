// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/debug.h"
#include "workshop.core/containers/string.h"

#include <array>
#include <mutex>
#include <sys/prctl.h>
#include <unistd.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <elfutils/libdwfl.h>

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

static char* s_debuginfo_path = nullptr;
static Dwfl_Callbacks s_callbacks = {
    .find_elf = dwfl_linux_proc_find_elf,
    .find_debuginfo = dwfl_standard_find_debuginfo,
    .debuginfo_path = &s_debuginfo_path,
};
static Dwfl* s_dwfl = nullptr;
static bool s_initialized = false;
static std::mutex g_dwfl_mutex;

Dwfl* get_dwfl_session()
{
    if (!s_initialized)
    {
        s_initialized = true;

        s_dwfl = dwfl_begin(&s_callbacks);
        if (s_dwfl != nullptr)
        {
            if (dwfl_linux_proc_report(s_dwfl, getpid()) != 0 ||
                dwfl_report_end(s_dwfl, nullptr, nullptr) != 0)
            {
                dwfl_end(s_dwfl);
                s_dwfl = nullptr;
            }
        }
    }

    return s_dwfl;
}

void resolve_source_location(void* address, std::string& filename, size_t& line)
{
    std::lock_guard lock(g_dwfl_mutex);

    Dwfl* dwfl = get_dwfl_session();
    if (dwfl == nullptr)
    {
        return;
    }

    Dwarf_Addr addr = reinterpret_cast<Dwarf_Addr>(address);

    Dwfl_Module* module = dwfl_addrmodule(dwfl, addr);
    if (module == nullptr)
    {
        return;
    }

    Dwfl_Line* dwfl_line = dwfl_module_getsrc(module, addr);
    if (dwfl_line == nullptr)
    {
        return;
    }

    int line_number = 0;
    const char* src_file = dwfl_lineinfo(dwfl_line, &addr, &line_number, nullptr, nullptr, nullptr);
    if (src_file != nullptr)
    {
        filename = src_file;
        line = static_cast<size_t>(line_number);
    }
}

};

namespace ws {

void db_set_thread_name(const std::string& name)
{
    prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
}

void db_break()
{
    db_flush();
#ifndef WS_RELEASE
    __builtin_trap();
#endif
}

void db_terminate()
{
    db_flush();
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

    constexpr size_t k_max_frames = 256;
    std::array<void*, k_max_frames> buffer;

    size_t skip_frames = frame_offset + 1;
    size_t capture_count = std::min(skip_frames + frame_count, k_max_frames);

    int captured_frames = backtrace(buffer.data(), static_cast<int>(capture_count));
    if (captured_frames <= static_cast<int>(skip_frames))
    {
        return result;
    }

    int wanted_frames = captured_frames - static_cast<int>(skip_frames);
    char** symbols = backtrace_symbols(buffer.data() + skip_frames, wanted_frames);

    for (int i = 0; i < wanted_frames; i++)
    {
        db_callstack::frame& frame = result->frames.emplace_back();
        frame.address = reinterpret_cast<size_t>(buffer[skip_frames + i]);
        frame.line = 0;

        resolve_source_location(buffer[skip_frames + i], frame.filename, frame.line);

        if (symbols != nullptr)
        {
            // backtrace_symbols format: "module(function+0xoffset) [0xaddress]"
            std::string symbol = symbols[i];

            size_t open_paren = symbol.find('(');
            size_t plus = symbol.find('+', open_paren);

            if (open_paren != std::string::npos)
            {
                frame.module = symbol.substr(0, open_paren);
            }
            if (open_paren != std::string::npos && plus != std::string::npos && plus > open_paren)
            {
                std::string mangled = symbol.substr(open_paren + 1, plus - open_paren - 1);

                int demangle_status = 0;
                char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &demangle_status);
                if (demangle_status == 0 && demangled != nullptr)
                {
                    frame.function = demangled;
                    free(demangled);
                }
                else
                {
                    frame.function = mangled;
                }
            }
        }
    }

    free(symbols);

    return result;
}

void db_move_console(size_t x, size_t y, size_t width, size_t height)
{
    // Not supported.
}

}; // namespace workshop
