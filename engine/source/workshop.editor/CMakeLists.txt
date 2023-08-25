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
    "editor/editor_main_menu.cpp"
    "editor/editor_main_menu.h"
    "editor/editor_window.cpp"
    "editor/editor_window.h"
    "editor/windows/editor_loading_window.cpp"
    "editor/windows/editor_loading_window.h"
    "editor/windows/editor_log_window.cpp"
    "editor/windows/editor_log_window.h"
    "editor/windows/editor_memory_window.cpp"
    "editor/windows/editor_memory_window.h"
    "editor/windows/editor_properties_window.cpp"
    "editor/windows/editor_properties_window.h"
    "editor/windows/editor_scene_tree_window.cpp"
    "editor/windows/editor_scene_tree_window.h"
    
    "editor/utils/allocation_tree.cpp"
    "editor/utils/allocation_tree.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    yamlcpp
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier2/editor")
