# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.windowing C CXX)

SET(SOURCES
    "window.h"
    "window.cpp"
    "windowing.h"
    "windowing.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Link appropriate implementation libraries.
target_link_libraries(${PROJECT_NAME} 
    workshop.windowing.sdl
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/windowing")
