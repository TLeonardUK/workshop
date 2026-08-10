# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================

# Disable unity builds for all third party code.
set(CMAKE_UNITY_BUILD OFF)

add_subdirectory(imgui)
add_subdirectory(sdl2)
add_subdirectory(nlohmann)
add_subdirectory(ImGuiColorTextEdit)
add_subdirectory(ImGuizmo)
add_subdirectory(pix)
add_subdirectory(yamlcpp)
add_subdirectory(lodepng)
add_subdirectory(bc7enc)
add_subdirectory(zlib)
add_subdirectory(dds-ktx)
add_subdirectory(assimp)
add_subdirectory(stb)
add_subdirectory(compressonator)
add_subdirectory(JoltPhysics)

add_subdirectory(dxcompiler)

add_subdirectory(nativefiledialog)