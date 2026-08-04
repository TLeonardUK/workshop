# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================

# Define our standard configuration modes.
set(CMAKE_CONFIGURATION_TYPES "Debug;Profile;Release") 

# Figure out architecture we are building for.
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
	set(ENV_ARCHITECTURE "x86" CACHE INTERNAL "")
else()
	set(ENV_ARCHITECTURE "x64" CACHE INTERNAL "")
endif()

# All projects write to the config appropriate output directory.
foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} config_upper)
    string(TOLOWER ${config} config_lower)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper} ${ENV_ROOT_PATH}/intermediate/libs/${ENV_ARCHITECTURE}_${config_lower})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper} ${ENV_ROOT_PATH}/bin/${ENV_ARCHITECTURE}_${config_lower})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${ENV_ROOT_PATH}/bin/${ENV_ARCHITECTURE}_${config_lower})
endforeach()

include(utils)
include(compiler)

# Include platform specific scripts
if (WIN32)
    include(platform.win32/win32)
endif()

include(compiler-late)
