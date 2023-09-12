# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

# The engine library contains the base game engine code, this contains and binds together the
# the basic building blocks of the engine into something capable of running and executing.
#
# High level gameplay constructs such as specialized objects, ai, etc. Should all exist in 
# a framework library, not in here.

project(workshop.engine C CXX)

SET(SOURCES
    "app/engine_app.cpp"
    "app/engine_app.h"

    "engine/engine.cpp"
    "engine/engine.h"
    "engine/world.cpp"
    "engine/world.h"

    "ecs/system.cpp"
    "ecs/system.h"
    "ecs/meta_component.h"
    "ecs/component.cpp"
    "ecs/component.h"
    "ecs/component_filter.cpp"
    "ecs/component_filter.h"
    "ecs/component_filter_archetype.cpp"
    "ecs/component_filter_archetype.h"
    "ecs/object.cpp"
    "ecs/object.h"
    "ecs/object_manager.cpp"
    "ecs/object_manager.h"
    
    "assets/asset_database.cpp"
    "assets/asset_database.h"
    
    "assets/scene/scene.cpp"
    "assets/scene/scene.h"
    "assets/scene/scene_loader.cpp"
    "assets/scene/scene_loader.h"
    
    "presentation/presenter.cpp"
    "presentation/presenter.h"
    
    "utils/stream.cpp"
    "utils/stream.h"
    "utils/yaml.cpp"
    "utils/yaml.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

set(NON_RELEASE_LIBS "")

# Include pix in windows/non-release configurations.
if (WIN32)
    set(NON_RELEASE_LIBS ${NON_RELEASE_LIBS} pix)
endif()

target_link_libraries(${PROJECT_NAME} 
    workshop.core
    workshop.renderer
    workshop.window_interface
    workshop.input_interface
    workshop.platform_interface
    workshop.physics_interface
    workshop.editor
    "$<$<CONFIG:Debug>:${NON_RELEASE_LIBS}>"
    "$<$<CONFIG:Profile>:${NON_RELEASE_LIBS}>"
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier2/engine")
