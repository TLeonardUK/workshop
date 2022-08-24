# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(pix C CXX)

add_library(${PROJECT_NAME} SHARED IMPORTED GLOBAL)
set_target_properties(${PROJECT_NAME} PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")

