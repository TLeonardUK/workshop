# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.windowing.sdl C CXX)

SET(SOURCES
    "sdl_window.h"
    "sdl_window.cpp"
    "sdl_windowing.h"
    "sdl_windowing.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/windowing")

target_link_libraries(${PROJECT_NAME} 
    sdl2
)
