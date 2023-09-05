# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================
project(workshop.core.win32 C CXX)

SET(SOURCES
    "containers/string.cpp"
    
    "debug/debug.cpp"

    "filesystem/file.cpp"
    "filesystem/path_watcher.cpp"
    
    "memory/memory.cpp"
    
    "platform/platform.cpp"
    
    "hashing/guid.cpp"

    "utils/windows_headers.h"
    
    "perf/profile.cpp"

    "entry.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier0/core")

target_link_libraries(${PROJECT_NAME}
    workshop.core
    dbghelp.lib 
    Rpcrt4.lib
)