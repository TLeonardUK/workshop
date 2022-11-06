# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(dxcompiler C CXX)

add_library(dxcompiler::main SHARED IMPORTED GLOBAL)
set_target_properties(dxcompiler::main PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}\\dxc_2022_07_18\\bin\\x64\\dxcompiler.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}\\dxc_2022_07_18\\lib\\x64\\dxcompiler.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}\\dxc_2022_07_18\\inc"
)

add_library(dxcompiler::dxil SHARED IMPORTED GLOBAL)
set_target_properties(dxcompiler::dxil PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}\\dxc_2022_07_18\\bin\\x64\\dxil.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}\\dxc_2022_07_18\\lib\\x64\\dxcompiler.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}\\dxc_2022_07_18\\inc"
)

add_library(${PROJECT_NAME} INTERFACE IMPORTED GLOBAL)
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERFACE_LINK_LIBRARIES dxcompiler::main dxcompiler::dxil)