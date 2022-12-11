# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.renderer C CXX)

SET(SOURCES
    "common_types.h"

    "renderer.h"
    "renderer.cpp"
    
    "render_world_state.h"
    "render_world_state.cpp"
    "render_graph.h"
    "render_graph.cpp"
    "render_effect.h"
    "render_effect.cpp"
    "render_output.h"
    "render_output.cpp"
    "render_pass.h"
    "render_pass.cpp"
    "render_pass_fullscreen.h"
    "render_pass_fullscreen.cpp"
    "render_system.h"
    "render_system.cpp"
    "render_world_state.h"
    "render_world_state.cpp"    
    "render_view.h"
    "render_view.cpp"
    
    "systems/render_system_clear.h"
    "systems/render_system_clear.cpp"
    "systems/render_system_resolve_backbuffer.h"
    "systems/render_system_resolve_backbuffer.cpp"
    
    "assets/shader/shader.h"
    "assets/shader/shader.cpp"
    "assets/shader/shader_loader.h"
    "assets/shader/shader_loader.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/rendering")

target_link_libraries(${PROJECT_NAME} 
    workshop.render_interface
    workshop.assets
)
