# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.window_interface.stub C CXX)

SET(SOURCES
    "stub_window.h"
    "stub_window.cpp"
    "stub_window_interface.h"
    "stub_window_interface.cpp"

    "public.pch"
    "private.pch"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/window")

target_link_libraries(${PROJECT_NAME}
    workshop.core
)

if (USE_PRECOMPILED_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PUBLIC public.pch PRIVATE private.pch)
endif()