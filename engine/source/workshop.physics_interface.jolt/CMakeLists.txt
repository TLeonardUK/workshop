# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.physics_interface.jolt C CXX)

SET(SOURCES
    "jolt_pi_interface.h"
    "jolt_pi_interface.cpp"
    
    "jolt_pi_world.h"
    "jolt_pi_world.cpp"
    "jolt_pi_job_system.h"
    "jolt_pi_job_system.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/physics")

target_link_libraries(${PROJECT_NAME} 
    workshop.core
    Jolt
)
