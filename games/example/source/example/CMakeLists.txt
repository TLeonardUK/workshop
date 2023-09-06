# ================================================================================================
#  roguelike
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================
project(example C CXX)

SET(SOURCES
    "app.cpp"
    "app.h"
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    workshop.core
    workshop.engine
    workshop.game_framework
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "games/example")

util_copy_all_dlls_to_output(${PROJECT_NAME})