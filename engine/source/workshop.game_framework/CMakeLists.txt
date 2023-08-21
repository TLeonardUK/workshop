# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

# The engine library contains generic game framework functionality - cameras/ai/etc.

project(workshop.game_framework C CXX)

SET(SOURCES
    "components/camera/camera_component.h"    
    "components/camera/fly_camera_movement_component.h"    
    "components/transform/transform_component.h"    
    
    "systems/camera/camera_system.h"  
    "systems/camera/camera_system.cpp"  
    "systems/camera/fly_camera_movement_system.cpp"
    "systems/camera/fly_camera_movement_system.h"  
    
    "systems/transform/transform_system.h"  
    "systems/transform/transform_system.cpp"    
    
    "systems/default_systems.h"    
    "systems/default_systems.cpp"    
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    workshop.engine
    yamlcpp
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier2/engine")
