// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/entry.h"
#include "workshop.core.win32/utils/windows_headers.h"

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

    // Magic mystery symbols that only exist on microsoft compilers...
    return ws::entry_point(__argc, __argv);
}
#endif