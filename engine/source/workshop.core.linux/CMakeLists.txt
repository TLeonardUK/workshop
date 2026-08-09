# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================
project(workshop.core.linux C CXX)

SET(SOURCES
    "containers/string.cpp"
    
    "debug/debug.cpp"

    "filesystem/file.cpp"
    "filesystem/async_io_manager.cpp"
    "filesystem/async_io_manager.h"
    "filesystem/path_watcher.cpp"
    
    "memory/memory.cpp"
    "memory/memory_hooks.cpp"
    
    "platform/platform.cpp"
    
    "hashing/guid.cpp"

    "perf/profile.cpp"

    "entry.cpp"

    "public.pch"
    "private.pch"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier0/core")

find_library(UUID_LIBRARY uuid REQUIRED)

find_path(DW_INCLUDE_DIR NAMES elfutils/libdwfl.h REQUIRED)
find_library(DW_LIBRARY NAMES dw REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${DW_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME}
    workshop.core
    SDL2::SDL2
    ${UUID_LIBRARY}
    ${DW_LIBRARY}
)

if (USE_PRECOMPILED_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PUBLIC public.pch PRIVATE private.pch)
endif()