# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(nativefiledialog C CXX)

SET(SOURCES
	"src/include/nfd.h"
)

if (WIN32)
	set(SOURCES ${SOURCES} "src/nfd_win.cpp")
elseif (LINUX)
	set(SOURCES ${SOURCES} "src/nfd_portal.cpp")
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/include")

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")

if (WIN32)
	target_link_libraries(${PROJECT_NAME} comctl32.lib)
elseif (LINUX)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(DBUS REQUIRED dbus-1)
	target_include_directories(${PROJECT_NAME} PRIVATE ${DBUS_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${DBUS_LIBRARIES})
endif()


