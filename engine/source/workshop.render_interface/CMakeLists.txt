# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.render_interface C CXX)

SET(SOURCES
    "ri_interface.h"
    "ri_interface.cpp"
    "ri_swapchain.h"
    "ri_fence.h"
    "ri_command_queue.h"
    "ri_command_list.h"
    "ri_texture.h"
    "ri_buffer.h"
    "ri_sampler.h"
    "ri_types.h"
    "ri_types.cpp"
    "ri_shader_compiler.h"
    "ri_texture_compiler.h"
    "ri_pipeline.h"
    "ri_query.h"
    "ri_raytracing_tlas.h"
    "ri_raytracing_blas.h"
    "ri_param_block.h"
    "ri_param_block.cpp"
    "ri_param_block_archetype.h"
    "ri_layout_factory.h"
    "ri_layout_factory.cpp"
    "ri_staging_buffer.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/rendering")

# Link appropriate implementation libraries.
target_link_libraries(${PROJECT_NAME} 
    workshop.core)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} workshop.render_interface.dx12)
endif()
