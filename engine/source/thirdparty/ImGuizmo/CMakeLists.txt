# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(imguizmo C CXX)

SET(SOURCES
    ImGuizmo.cpp
    GraphEditor.cpp
    ImCurveEdit.cpp
    ImGradient.cpp
    ImSequencer.cpp

    ImGuizmo.h
    GraphEditor.h
    ImCurveEdit.h
    ImGradient.h
    ImSequencer.h
    ImZoomSlider.h
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../imgui/)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")


