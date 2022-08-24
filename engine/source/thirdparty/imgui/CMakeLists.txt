# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(imgui C CXX)

SET(SOURCES
	"imconfig.h"
	"imgui.cpp"
	"imgui.h"
	"imgui_demo.cpp"
	"imgui_draw.cpp"
	"imgui_internal.h"
	"imgui_tables.cpp"
	"imgui_widgets.cpp"
	"imstb_rectpack.h"
	"imstb_textedit.h"
	"imstb_truetype.h"
    "backends/imgui_impl_sdl.cpp"
    "backends/imgui_impl_sdl.h"
#    "backends/imgui_impl_sdlrenderer.cpp"
#    "backends/imgui_impl_sdlrenderer.h"
    "backends/imgui_impl_opengl3.cpp"
    "backends/imgui_impl_opengl3.h"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")

target_link_libraries(${PROJECT_NAME} sdl2)


