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
    
    "containers/memory_heap.cpp"
    "containers/memory_heap.h"
    "containers/command_queue.cpp"
    "containers/command_queue.h"
    "containers/byte_queue.cpp"
    "containers/byte_queue.h"
    "containers/json.cpp"
    "containers/json.h"
    "containers/string.cpp"
    "containers/string.h"
    "containers/oct_tree.cpp"
    "containers/oct_tree.h"
    
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
    "filesystem/path_watcher.cpp"
    "filesystem/path_watcher.h"
    "filesystem/stream.h"
    "filesystem/stream.cpp"
    "filesystem/disk_stream.h"
    "filesystem/disk_stream.cpp"
    "filesystem/virtual_file_system_types.h"
    "filesystem/virtual_file_system.h"
    "filesystem/virtual_file_system.cpp"
    "filesystem/virtual_file_system_disk_handler.h"
    "filesystem/virtual_file_system_disk_handler.cpp"
    "filesystem/virtual_file_system_redirect_handler.h"
    "filesystem/virtual_file_system_redirect_handler.cpp"
    "filesystem/virtual_file_system_handler.h"
    "filesystem/virtual_file_system_handler.cpp"

    "hashing/guid.cpp"
    "hashing/guid.h"
    "hashing/hash.h"
    "hashing/string_hash.cpp"
    "hashing/string_hash.h"
    
    "platform/platform.h"

    "perf/profile.h"
    "perf/timer.h"
    "perf/timer.cpp"
    
    "utils/traits.h"
    "utils/event.h"
    "utils/init_list.cpp"
    "utils/init_list.h"
    "utils/result.h"
    "utils/singleton.h"
    "utils/version.cpp"
    "utils/version.h"    
    "utils/frame_time.cpp"
    "utils/frame_time.h" 
    "utils/time.cpp"
    "utils/time.h"
    "utils/string_formatter.h"
    
    "math/math.h"
    "math/math.cpp"
    "math/aabb.h"
    "math/cylinder.h"
    "math/frustum.h"
    "math/matrix2.h"
    "math/matrix3.h"
    "math/matrix4.h"
    "math/obb.h"
    "math/plane.h"
    "math/quat.h"
    "math/rect.h"
    "math/sphere.h"
    "math/hemisphere.h"
    "math/vector2.h"
    "math/vector3.h"
    "math/vector4.h"
    
    "drawing/color.h"
    "drawing/pixmap.h"
    "drawing/pixmap.cpp"
    "drawing/pixmap_png_loader.h"
    "drawing/pixmap_png_loader.cpp"
    "drawing/pixmap_dds_loader.h"
    "drawing/pixmap_dds_loader.cpp"
    "drawing/pixmap_stb_loader.h"
    "drawing/pixmap_stb_loader.cpp"
    
    "geometry/geometry.h"
    "geometry/geometry.cpp"
    "geometry/geometry_assimp_loader.h"
    "geometry/geometry_assimp_loader.cpp"
    
    "memory/memory_tracker.h"
    "memory/memory_tracker.cpp"
    
    "statistics/statistics_manager.h"
    "statistics/statistics_manager.cpp"

    "entry.cpp"
    "entry.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier0/core")

set(PLATFORM_LIB "")

# Link to appropriate platform implementation.
if (WIN32)
    set(PLATFORM_LIB ${PLATFORM_LIB} workshop.core.win32)
endif()

target_link_libraries(${PROJECT_NAME} 
    lodepng
    bc7enc
    assimp
    zlib
    dds-ktx
    stb
    CMP_Core
    ${PLATFORM_LIB}
)
