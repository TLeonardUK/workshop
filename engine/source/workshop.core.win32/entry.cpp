// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/entry.h"
#include "workshop.core.win32/utils/windows_headers.h"
#include "workshop.core/memory/memory.h"
#include "workshop.core/memory/memory_tracker.h"

namespace ws
{
    void install_memory_hooks();
};

#if 0
int main(int argc, char* argv[])
{
    return ws::entry_point(argc, argv);
}
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifndef WS_RELEASE
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
#endif

    // Hook memory functions as early as possible.
    ws::memory_tracker mem_tracker;
#ifndef WS_DEBUG // We use the crt debug heap in debug, the hooks complicate things so we just don't use them here.
    ws::install_memory_hooks();
#endif

    // Magic mystery symbols that only exist on microsoft compilers...
    return ws::entry_point(__argc, __argv);
}
#endif