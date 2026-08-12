# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (USE_UNITY_BUILDS)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 12)
endif()

set(COMPILE_OPTIONS "")
set(LINK_OPTIONS "")
set(DEBUG_COMPILE_OPTIONS "")
set(PROFILE_COMPILE_OPTIONS "")
set(RELEASE_COMPILE_OPTIONS "")
set(DEBUG_LINK_OPTIONS "")
set(PROFILE_LINK_OPTIONS "")
set(RELEASE_COMPILE_OPTIONS "")

# Architecture define.
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_X86)
else()
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_X64)
endif()

# Compile defines
set(DEBUG_COMPILE_OPTIONS   ${COMPILE_OPTIONS} -DWS_DEBUG)
set(PROFILE_COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_PROFILE)
set(RELEASE_COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_RELEASE)

# Remove RTC flags - we don't use them as they are imcompatible with different optimization levels 
# in different source files.
STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}") 
STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")