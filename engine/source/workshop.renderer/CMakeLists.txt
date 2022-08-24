# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.renderer C CXX)

SET(SOURCES
    "renderer.h"
    "renderer.cpp"
    
    "render_world_state.h"
    "render_world_state.cpp"
    "render_effect.h"
    "render_effect.cpp"
    "render_graph.h"
    "render_graph.cpp"
    "render_pass.h"
    "render_pass.cpp"
    "render_pass_graphics.h"
    "render_pass_graphics.cpp"
    "render_pass_compute.h"
    "render_pass_compute.cpp"
    "render_system.h"
    "render_system.cpp"
    "render_world_state.h"
    "render_world_state.cpp"

    "systems/render_system_geometry.h"
    "systems/render_system_geometry.cpp"
    "systems/render_system_copy_to_backbuffer.h"
    "systems/render_system_copy_to_backbuffer.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/rendering")

target_link_libraries(${PROJECT_NAME} workshop.render_interface)
