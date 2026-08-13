// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#define VK_ONLY_EXPORTED_PROTOTYPES

#include <vulkan/vulkan.h>

#include "thirdparty/sdl2/include/SDL_vulkan.h"

#include "workshop.core/debug/debug.h"

namespace ws {

// Handy macro for checking the result of a vulkan call and asserting/logging on failure.
#define vk_check(x) \
    do { \
        VkResult vk_check_result = (x); \
        if (vk_check_result != VK_SUCCESS) \
        { \
            db_error(render_interface, "Vulkan call failed with result %i: %s", static_cast<int>(vk_check_result), #x); \
            db_assert_message(vk_check_result == VK_SUCCESS, "Vulkan call failed: %s", #x); \
        } \
    } while (0)

}; // namespace ws
