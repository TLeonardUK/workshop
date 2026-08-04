# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================
cmake_minimum_required(VERSION 3.8)

if (POLICY CMP0141)
    # Allow debug information format flags to be selected by abstraction.
    cmake_policy(SET CMP0141 NEW)
endif()

# Environment settings that all projects inherit.
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/engine/tools/build;${CMAKE_MODULE_PATH}")
set(ENV_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/engine/source")
set(ENV_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(USE_PRECOMPILED_HEADERS 1)
set(USE_UNITY_BUILDS 1)

# Common configuration for all projects.
include(build)

project(workshop C CXX)

add_subdirectory(engine)
add_subdirectory(games)

