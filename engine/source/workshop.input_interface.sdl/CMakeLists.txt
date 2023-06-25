# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.input_interface.sdl C CXX)

SET(SOURCES
    "sdl_input_interface.h"
    "sdl_input_interface.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/input")

target_link_libraries(${PROJECT_NAME} 
    sdl2
)
