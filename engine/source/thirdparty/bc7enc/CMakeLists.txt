# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(bc7enc C CXX)

SET(SOURCES
	"bc7decomp.cpp"
	"bc7decomp.h"
	"bc7enc.c"
	"bc7enc.h"
	"dds_defs.h"
	"rgbcx.h"
	"rgbcx.cpp"
	"rgbcx_table4.h"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")


