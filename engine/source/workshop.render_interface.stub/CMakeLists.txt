# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.render_interface.stub C CXX)

SET(SOURCES
    "stub_ri_interface.h"
    "stub_ri_interface.cpp"
    "stub_ri_swapchain.h"
    "stub_ri_swapchain.cpp"
    "stub_ri_fence.h"
    "stub_ri_fence.cpp"
    "stub_ri_command_queue.h"
    "stub_ri_command_queue.cpp"
    "stub_ri_command_list.h"
    "stub_ri_command_list.cpp"
    "stub_ri_texture.h"
    "stub_ri_texture.cpp"
    "stub_ri_buffer.h"
    "stub_ri_buffer.cpp"
    "stub_ri_query.h"
    "stub_ri_query.cpp"
    "stub_ri_layout_factory.h"
    "stub_ri_layout_factory.cpp"
    "stub_ri_sampler.h"
    "stub_ri_sampler.cpp"
    "stub_ri_shader_compiler.h"
    "stub_ri_shader_compiler.cpp"
    "stub_ri_texture_compiler.h"
    "stub_ri_texture_compiler.cpp"
    "stub_ri_pipeline.h"
    "stub_ri_pipeline.cpp"
    "stub_ri_param_block.h"
    "stub_ri_param_block.cpp"
    "stub_ri_param_block_archetype.h"
    "stub_ri_param_block_archetype.cpp"
    "stub_ri_staging_buffer.h"
    "stub_ri_staging_buffer.cpp"
    "stub_ri_raytracing_tlas.h"
    "stub_ri_raytracing_tlas.cpp"
    "stub_ri_raytracing_blas.h"
    "stub_ri_raytracing_blas.cpp"

    "public.pch"
    "private.pch"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/rendering")

target_link_libraries(${PROJECT_NAME}
    workshop.core
)

if (USE_PRECOMPILED_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PUBLIC public.pch PRIVATE private.pch)
endif()
