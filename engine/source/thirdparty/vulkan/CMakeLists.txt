# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(vulkan C CXX)

set(VULKAN_VERSION "1.4.357.0")

add_library(${PROJECT_NAME} STATIC IMPORTED GLOBAL)

set_target_properties(${PROJECT_NAME} PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/${VULKAN_VERSION}/Lib/vulkan-1.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/${VULKAN_VERSION}/Include"
)


