# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.render_interface.dx12 C CXX)

SET(SOURCES
    "dx12_ri_interface.h"
    "dx12_ri_interface.cpp"
    "dx12_ri_swapchain.h"
    "dx12_ri_swapchain.cpp"
    "dx12_ri_fence.h"
    "dx12_ri_fence.cpp"
    "dx12_ri_descriptor_heap.h"
    "dx12_ri_descriptor_heap.cpp"
    "dx12_ri_descriptor_table.h"
    "dx12_ri_descriptor_table.cpp"
    "dx12_ri_command_queue.h"
    "dx12_ri_command_queue.cpp"
    "dx12_ri_command_list.h"
    "dx12_ri_command_list.cpp"
    "dx12_ri_texture.h"
    "dx12_ri_texture.cpp"
    "dx12_ri_buffer.h"
    "dx12_ri_buffer.cpp"
    "dx12_ri_layout_factory.h"
    "dx12_ri_layout_factory.cpp"    
    "dx12_ri_sampler.h"
    "dx12_ri_sampler.cpp"
    "dx12_ri_shader_compiler.h"
    "dx12_ri_shader_compiler.cpp"
    "dx12_ri_pipeline.h"
    "dx12_ri_pipeline.cpp"
    "dx12_ri_param_block.h"
    "dx12_ri_param_block.cpp"
    "dx12_ri_param_block_archetype.h"
    "dx12_ri_param_block_archetype.cpp"
    "dx12_ri_upload_manager.h"
    "dx12_ri_upload_manager.cpp"
    "dx12_types.h"
    "dx12_types.cpp"
    "dx12_headers.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/rendering")

target_link_libraries(${PROJECT_NAME} 
    sdl2
    dxcompiler
    d3d12.lib
    dxgi.lib
)

