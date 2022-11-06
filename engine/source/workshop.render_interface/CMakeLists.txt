# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.render_interface C CXX)

SET(SOURCES
    "render_interface.h"
    "render_interface.cpp"
    "render_swapchain.h"
    "render_fence.h"
    "render_command_queue.h"
    "render_command_list.h"
    "render_target.h"
    "render_types.h"
    "render_shader_compiler.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/rendering")

# Link appropriate implementation libraries.
if (WIN32)
    target_link_libraries(${PROJECT_NAME} workshop.render_interface.dx12)
endif()
