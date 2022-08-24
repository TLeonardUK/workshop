# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================
project(nlohmann C CXX)

SET(SOURCES
	"json.hpp"
)

add_custom_target(${PROJECT_NAME} SOURCES ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")

