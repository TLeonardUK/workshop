# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.physics_interface C CXX)

SET(SOURCES
    "physics_interface.h"
    "physics_interface.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Link appropriate implementation libraries.
target_link_libraries(${PROJECT_NAME} 
    workshop.core
    workshop.physics_interface.jolt
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/physics")
