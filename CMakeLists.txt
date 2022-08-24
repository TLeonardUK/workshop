# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================
cmake_minimum_required(VERSION 3.8)

# Environment settings that all projects inherit.
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/engine/tools/build;${CMAKE_MODULE_PATH}")
set(ENV_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/engine/source")
set(ENV_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# Common configuration for all projects.
include(common)

project(workshop C CXX)

add_subdirectory(engine)
add_subdirectory(games)



