# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.render_interface.dx12 C CXX)

SET(SOURCES
    "dx12_render_interface.h"
    "dx12_render_interface.cpp"
    "dx12_render_swapchain.h"
    "dx12_render_swapchain.cpp"
    "dx12_render_fence.h"
    "dx12_render_fence.cpp"
    "dx12_render_descriptor_heap.h"
    "dx12_render_descriptor_heap.cpp"
    "dx12_render_command_queue.h"
    "dx12_render_command_queue.cpp"
    "dx12_render_command_list.h"
    "dx12_render_command_list.cpp"
    "dx12_render_target.h"
    "dx12_render_target.cpp"
    "dx12_render_shader_compiler.h"
    "dx12_render_shader_compiler.cpp"
    "dx12_types.h"
    "dx12_types.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/rendering")

target_link_libraries(${PROJECT_NAME} 
    sdl2
    dxcompiler
    d3d12.lib
    dxgi.lib
)

