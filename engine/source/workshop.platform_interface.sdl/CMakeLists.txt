# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.platform_interface.sdl C CXX)

SET(SOURCES
    "sdl_platform_interface.h"
    "sdl_platform_interface.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/platform")

target_link_libraries(${PROJECT_NAME} 
    workshop.core
    sdl2
)
