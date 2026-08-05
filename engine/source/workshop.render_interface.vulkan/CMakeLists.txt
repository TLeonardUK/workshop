# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.render_interface.vulkan C CXX)

SET(SOURCES
    "vulkan_ri_interface.h"
    "vulkan_ri_interface.cpp"
    "vulkan_ri_swapchain.h"
    "vulkan_ri_swapchain.cpp"
    "vulkan_ri_fence.h"
    "vulkan_ri_fence.cpp"
    "vulkan_ri_command_queue.h"
    "vulkan_ri_command_queue.cpp"
    "vulkan_ri_command_list.h"
    "vulkan_ri_command_list.cpp"
    "vulkan_ri_texture.h"
    "vulkan_ri_texture.cpp"
    "vulkan_ri_buffer.h"
    "vulkan_ri_buffer.cpp"
    "vulkan_ri_query.h"
    "vulkan_ri_query.cpp"
    "vulkan_ri_layout_factory.h"
    "vulkan_ri_layout_factory.cpp"    
    "vulkan_ri_sampler.h"
    "vulkan_ri_sampler.cpp"
    "vulkan_ri_shader_compiler.h"
    "vulkan_ri_shader_compiler.cpp"
    "vulkan_ri_texture_compiler.h"
    "vulkan_ri_texture_compiler.cpp"
    "vulkan_ri_pipeline.h"
    "vulkan_ri_pipeline.cpp"
    "vulkan_ri_param_block.h"
    "vulkan_ri_param_block.cpp"
    "vulkan_ri_param_block_archetype.h"
    "vulkan_ri_param_block_archetype.cpp"
    "vulkan_ri_staging_buffer.h"
    "vulkan_ri_staging_buffer.cpp"
    
    "vulkan_ri_raytracing_tlas.h"
    "vulkan_ri_raytracing_tlas.cpp"
    "vulkan_ri_raytracing_blas.h"
    "vulkan_ri_raytracing_blas.cpp"
    "vulkan_types.h"
    "vulkan_types.cpp"
    "vulkan_headers.h"
    
    "public.pch"
    "private.pch"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/rendering")

target_link_libraries(${PROJECT_NAME} 
    workshop.core
    SDL2::SDL2
)

if (USE_PRECOMPILED_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PUBLIC public.pch PRIVATE private.pch)
endif()