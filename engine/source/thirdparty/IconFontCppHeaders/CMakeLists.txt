# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================
project(IconFontCppHeaders C CXX)

SET(SOURCES
	"IconsFontaudio.h"
	"IconsFontAwesome4.h"
	"IconsFontAwesome5.h"
	"IconsFontAwesome5Brands.h"
	"IconsFontAwesome5Pro.h"
	"IconsFontAwesome5ProBrands.h"
	"IconsForkAwesome.h"
	"IconsKenney.h"
	"IconsMaterialDesign.h"
)

add_custom_target(${PROJECT_NAME} SOURCES ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")

