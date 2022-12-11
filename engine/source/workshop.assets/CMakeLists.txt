# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.assets C CXX)

SET(SOURCES
    "asset.h"
    "asset.cpp"
    "asset_cache.h"
    "asset_cache.cpp"
    "asset_loader.h"
    "asset_loader.cpp"
    "asset_manager.h"
    "asset_manager.cpp"
    "caches/asset_cache_disk.h"
    "caches/asset_cache_disk.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    yamlcpp
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/loading")
