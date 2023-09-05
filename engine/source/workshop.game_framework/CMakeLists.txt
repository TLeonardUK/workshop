# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

# The engine library contains generic game framework functionality - cameras/ai/etc.

project(workshop.game_framework C CXX)

SET(SOURCES
    "components/camera/camera_component.h"    
    "components/camera/fly_camera_movement_component.h"    
    "components/lighting/spot_light_component.h"    
    "components/lighting/point_light_component.h"    
    "components/lighting/directional_light_component.h"    
    "components/lighting/light_probe_grid_component.h"    
    "components/lighting/light_component.h"         
    "components/lighting/reflection_probe_component.h"         
    "components/geometry/static_mesh_component.h"    
    "components/transform/transform_component.h"     
    "components/transform/bounds_component.h"    
    
    "systems/camera/camera_system.h"  
    "systems/camera/camera_system.cpp"  
    "systems/camera/fly_camera_movement_system.cpp"
    "systems/camera/fly_camera_movement_system.h"  
        
    "systems/lighting/light_system.h"  
    "systems/lighting/light_system.cpp"  
    "systems/lighting/directional_light_system.h"  
    "systems/lighting/directional_light_system.cpp"  
    "systems/lighting/point_light_system.h"  
    "systems/lighting/point_light_system.cpp"
    "systems/lighting/spot_light_system.h"  
    "systems/lighting/spot_light_system.cpp"  
    "systems/lighting/light_probe_grid_system.h"  
    "systems/lighting/light_probe_grid_system.cpp"
    "systems/lighting/reflection_probe_system.h"  
    "systems/lighting/reflection_probe_system.cpp"  

    "systems/geometry/static_mesh_system.h"  
    "systems/geometry/static_mesh_system.cpp"  

    "systems/transform/transform_system.h"  
    "systems/transform/transform_system.cpp"    
    "systems/transform/bounds_system.h"  
    "systems/transform/bounds_system.cpp"    
    
    "systems/default_systems.h"    
    "systems/default_systems.cpp"    
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    workshop.core
    workshop.engine
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier2/engine")
