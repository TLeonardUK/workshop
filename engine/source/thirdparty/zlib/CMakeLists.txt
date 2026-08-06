# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(zlib C CXX)

SET(SOURCES
    zlib.h
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h

    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)

if (NOT WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DHAVE_UNISTD_H=1)
endif()
target_compile_definitions(${PROJECT_NAME} PUBLIC -DHAVE_STDARG_H=1)
target_compile_definitions(${PROJECT_NAME} PUBLIC -D_LARGEFILE64_SOURCE=1)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")


