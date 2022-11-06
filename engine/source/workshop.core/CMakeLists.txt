# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

# The core library contains all low-level constructs that are agnostic of the game, things like
# containers, file-io, hashing, utils, etc.

project(workshop.core C CXX)

SET(SOURCES
    "app/app.cpp"
    "app/app.h"

    "async/async.h"
    "async/async.cpp"
    "async/task_scheduler.h"
    "async/task_scheduler.cpp"

    "containers/byte_queue.cpp"
    "containers/byte_queue.h"
    "containers/json.cpp"
    "containers/json.h"
    "containers/string.cpp"
    "containers/string.h"
    
    "debug/log.h"
    "debug/debug.cpp"
    "debug/debug.h"
    "debug/log_handler.cpp"
    "debug/log_handler.h"
    "debug/log_handler_console.cpp"
    "debug/log_handler_console.h"
    "debug/log_handler_file.cpp"
    "debug/log_handler_file.h"
    
    "filesystem/file.cpp"
    "filesystem/file.h"
    "filesystem/stream.h"
    "filesystem/stream.cpp"
    "filesystem/disk_stream.h"
    "filesystem/disk_stream.cpp"
    "filesystem/virtual_file_system_types.h"
    "filesystem/virtual_file_system.h"
    "filesystem/virtual_file_system.cpp"
    "filesystem/virtual_file_system_disk_handler.h"
    "filesystem/virtual_file_system_disk_handler.cpp"
    "filesystem/virtual_file_system_aliased_disk_handler.h"
    "filesystem/virtual_file_system_aliased_disk_handler.cpp"
    "filesystem/virtual_file_system_handler.h"
    "filesystem/virtual_file_system_handler.cpp"

    "hashing/guid.cpp"
    "hashing/guid.h"
    "hashing/hash.h"
    
    "platform/platform.h"

    "perf/profile.h"

    "utils/event.h"
    "utils/init_list.cpp"
    "utils/init_list.h"
    "utils/result.h"
    "utils/singleton.h"
    "utils/version.cpp"
    "utils/version.h"    
    "utils/frame_time.cpp"
    "utils/frame_time.h"
    
    "math/math.h"
    "math/math.cpp"
    
    "drawing/color.h"

    "entry.cpp"
    "entry.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/core")

# Link to appropriate platform implementation.
if (WIN32)
    target_link_libraries(${PROJECT_NAME} workshop.core.win32)
endif()
