# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(sdl2 C CXX)

add_library(${PROJECT_NAME} SHARED IMPORTED GLOBAL)
set_target_properties(${PROJECT_NAME} PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/SDL2.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/SDL2.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

#target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
#target_link_libraries(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/SDL2.lib")

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")


# Add a custom target to copy the dll to the binary folder, 

#add_custom_target(copy_sdl2_dll ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib/x64/SDL2.dll)

#add_custom_command(
#    TARGET copy_sdl2_dll
#    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SDL2.dll
#    COMMAND ${CMAKE_COMMAND} copy
#            ${CMAKE_CURRENT_BINARY_DIR}/lib/x64/SDL2.dll
#            ${CMAKE_SOURCE_DIR}
#)

#add_dependencies(${PROJECT_NAME} copy_sdl2_dll)

