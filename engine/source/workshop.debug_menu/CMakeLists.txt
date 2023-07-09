# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.debug_menu C CXX)

SET(SOURCES
    "debug_menu.h"
    "debug_menu.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/debug")

target_link_libraries(${PROJECT_NAME} 
    workshop.render_interface
    workshop.input_interface
    imgui
    yamlcpp
)
