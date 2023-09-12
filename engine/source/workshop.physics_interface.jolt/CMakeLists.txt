# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.physics_interface.jolt C CXX)

SET(SOURCES
    "jolt_physics_interface.h"
    "jolt_physics_interface.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/physics")

target_link_libraries(${PROJECT_NAME} 
    workshop.core
    Jolt
)
