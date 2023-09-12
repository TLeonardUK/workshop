# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(JoltPhysics C CXX)

add_subdirectory(Build)

target_include_directories(Jolt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

util_setup_folder_structure(Jolt SOURCES "engine/thirdparty")

