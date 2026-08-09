// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/entry.h"
#include "workshop.core/memory/memory.h"
#include "workshop.core/memory/memory_tracker.h"

namespace ws
{
    void install_memory_hooks();
};

int main(int argc, char* argv[])
{
    ws::memory_tracker mem_tracker;
    ws::install_memory_hooks();

    return ws::entry_point(argc, argv);
}
