# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================

include_directories("${ENV_ROOT_PATH}/engine/source")

add_subdirectory(workshop.core)
if (WIN32)
    add_subdirectory(workshop.core.win32)
endif()

add_subdirectory(workshop.engine)

add_subdirectory(workshop.render_interface)
if (WIN32)
    add_subdirectory(workshop.render_interface.dx12)
endif()
add_subdirectory(workshop.renderer)

add_subdirectory(workshop.windowing)
add_subdirectory(workshop.windowing.sdl)

add_subdirectory(thirdparty)


