# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(ImGuiColorTextEdit C CXX)

SET(SOURCES
	"TextEditor.cpp"
	"TextEditor.h"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")

target_link_libraries(${PROJECT_NAME} imgui)


