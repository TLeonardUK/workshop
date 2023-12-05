// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/debug/debug.h"
#include "workshop.core/containers/string.h"
#include "workshop.core.win32/utils/windows_headers.h"

#include <array>
#include <queue>

// If se async writing will happen on a background thread to avoid spikes when writing to output.
#define USE_ASYNC_CONSOLE_LOGGING 1

namespace {

const std::array<int, static_cast<int>(ws::console_color::count)> console_color_attributes = {
    0,  // unset
    12, // red
    14, // yellow
    10, // green
    07, // grey
    37, // white
};

};

namespace ws {

void db_set_thread_name(const std::string& name)
{
    std::wstring wide_name = widen_string(name);
    SetThreadDescription(GetCurrentThread(), wide_name.c_str());
}

void db_break()
{
#ifndef WS_RELEASE
    __debugbreak();
#endif
}

void db_terminate()
{
    std::terminate();
}

void db_console_write(const char* text, console_color color)
{
    if (color != console_color::unset)
    {
        static HANDLE console_handle = INVALID_HANDLE_VALUE;
        if (console_handle == INVALID_HANDLE_VALUE)
        {
            console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        }
        if (console_handle != INVALID_HANDLE_VALUE)
        {
            SetConsoleTextAttribute(console_handle, console_color_attributes[static_cast<int>(color)]);
        }
    }

    OutputDebugStringA(text);
    printf("%s", text);
}

result<void> db_load_symbols()
{
    db_verbose(core, "Loading symbols.");

    bool result = SymInitialize(GetCurrentProcess(), nullptr, true);
    if (!result)
    {
        db_warning(core, "Failed to load symbols for current process.");
    }
    else
    {
        db_verbose(core, "Symbols loaded successfully.");
    }

    return result;
}

result<void> db_unload_symbols()
{
    db_verbose(core, "Unloading symbols.");

    bool result = SymCleanup(GetCurrentProcess());
    if (!result)
    {
        db_warning(core, "Failed to unload symbols for current process.");
    }
    else
    {
        db_verbose(core, "Symbols unloaded successfully.");
    }

    return result;
}

std::unique_ptr<db_callstack> db_capture_callstack(size_t frame_offset, size_t frame_count)
{
    constexpr size_t k_max_frames = 256;
    std::array<void*, k_max_frames> frames;

    USHORT captured_frames = RtlCaptureStackBackTrace(
        static_cast<ULONG>(frame_offset + 1), 
        static_cast<ULONG>(std::min(frame_count, k_max_frames)),
        frames.data(),
        nullptr);

    std::unique_ptr<db_callstack> result = std::make_unique<db_callstack>();
    result->frames.resize(captured_frames);

    HANDLE process = GetCurrentProcess();
    DWORD displacement = 0;
    DWORD64 displacement64 = 0;

    for (size_t i = 0; i < captured_frames; i++)
    {
        db_callstack::frame& frame = result->frames[i];
        frame.address = reinterpret_cast<size_t>(frames[i]);

        char buffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_SYM_NAME * sizeof(TCHAR)];

        IMAGEHLP_SYMBOL64* symbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>(buffer);
        memset(buffer, 0, sizeof(buffer));
        symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        symbol->MaxNameLength = MAX_SYM_NAME;
        if (SymGetSymFromAddr64(process, frame.address, &displacement64, symbol))
        {
            frame.function = symbol->Name;
        }

        IMAGEHLP_MODULE64* module = reinterpret_cast<IMAGEHLP_MODULE64*>(buffer);
        memset(buffer, 0, sizeof(buffer));
        module->SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        if (SymGetModuleInfo64(process, frame.address, module))
        {
            frame.module = module->ModuleName;
        }

        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(line);
        if (SymGetLineFromAddr64(process, frame.address, &displacement, &line))
        {
            frame.filename = line.FileName;
            frame.line = line.LineNumber;
        }
    }

    return std::move(result);
}

void db_move_console(size_t x, size_t y, size_t width, size_t height)
{
    HWND hwnd = GetConsoleWindow();

    RECT window_rect;
    GetWindowRect(hwnd, &window_rect);

    if (x == 0)
    {
        x = window_rect.left;
    }
    if (y == 0)
    {
        y = window_rect.top;
    }
    if (width == 0)
    {
        width = window_rect.right - window_rect.left;
    }
    if (height == 0)
    {
        height = window_rect.bottom - window_rect.top;
    }

    MoveWindow(hwnd, (int)x, (int)y, (int)width, (int)height, true);
}

}; // namespace workshop
