# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(nativefiledialog C CXX)

SET(SOURCES
	"src/nfd_win.cpp"
	"src/include/nfd.h"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/include")

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")

target_link_libraries(${PROJECT_NAME} comctl32.lib)


