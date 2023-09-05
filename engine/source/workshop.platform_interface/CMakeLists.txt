# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.platform_interface C CXX)

SET(SOURCES
    "platform_interface.h"
    "platform_interface.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Link appropriate implementation libraries.
target_link_libraries(${PROJECT_NAME} 
    workshop.core
    workshop.platform_interface.sdl
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/platform")
