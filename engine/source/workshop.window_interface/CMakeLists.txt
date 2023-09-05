# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.window_interface C CXX)

SET(SOURCES
    "window.h"
    "window.cpp"
    "window_interface.h"
    "window_interface.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Link appropriate implementation libraries.
target_link_libraries(${PROJECT_NAME} 
    workshop.core
    workshop.window_interface.sdl
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/window")
