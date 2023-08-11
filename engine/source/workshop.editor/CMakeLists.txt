# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

# The editor library contains all the basic ui constructs required to display
# editor releated functionality.

project(workshop.editor C CXX)

SET(SOURCES
    "editor/editor.cpp"
    "editor/editor.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    yamlcpp
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier2/editor")
