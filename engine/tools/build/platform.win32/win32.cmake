# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_SYSTEM_VERSION 10.0.19041.0)
set(WINDOWS_KIT_BIN_DIR "C:\\Program Files (x86)\\Windows Kits\\10\\Bin\\10.0.19041.0\\x64")
set(WINDOWS_KIT_LIB_DIR "C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.19041.0\\um\\x64")
set(WINDOWS_KIT_INCLUDE_DIR "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.19041.0\\um")

# Disable secure CRT warnings on windows, our code is designed to be platform agnostic
# and these get in the way of that.
set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -D_CRT_SECURE_NO_WARNINGS=1)
set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -D_CRT_SECURE_NO_WARNINGS_GLOBALS=1)

# Ensure we compile in parallel.
set(COMPILE_OPTIONS ${COMPILE_OPTIONS} /MP)

# Use embedded debug info.
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,Profile>:Embedded>")

# Platform type define.
set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_WINDOWS)

set(COMPILE_OPTIONS ${COMPILE_OPTIONS} /WX)

set(COMPILE_OPTIONS ${COMPILE_OPTIONS} /Zc:preprocessor)

# Enable pix on all non-release builds.
set(DEBUG_COMPILE_OPTIONS   ${DEBUG_COMPILE_OPTIONS} -DUSE_PIX)
set(PROFILE_COMPILE_OPTIONS ${PROFILE_COMPILE_OPTIONS} -DUSE_PIX)    
