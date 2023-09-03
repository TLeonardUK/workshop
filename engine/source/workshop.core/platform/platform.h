// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/containers/string.h"
#include "workshop.core/math/math.h"

namespace ws {

enum class platform_type
{
    windows,

    COUNT
};

static const char* platform_type_strings[static_cast<int>(platform_type::COUNT)] = {
    "windows"
};

DEFINE_ENUM_TO_STRING(platform_type, platform_type_strings);

enum class config_type
{
    debug,
    profile,
    release,

    COUNT
};

static const char* config_type_strings[static_cast<int>(config_type::COUNT)] = {
    "debug",
    "profile",
    "release"
};

DEFINE_ENUM_TO_STRING(config_type, config_type_strings);

// Gets the platform the application is running on.
platform_type get_platform();

// Gets the configuration the application is running under.
config_type get_config();

// Gets the current memory usage, in bytes.
size_t get_memory_usage();

// Gets the amount, in bytes,  of the page file currently being used.
size_t get_pagefile_usage();

// Type of message dialog to display when calling message_dialog, this dictates
// the title/icon/etc that is shown in the dialog.
enum class message_dialog_type
{
    error,
    warning,
    message
};

// Shows a message to the user with the native message dialog.
void message_dialog(const char* text, message_dialog_type type);

}; // namespace ws::math