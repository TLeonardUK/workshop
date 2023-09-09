// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/ImGuizmo/ImGuizmo.h"
#include "thirdparty/IconFontCppHeaders/IconsFontAwesome5.h"

namespace ws {

// Button that can be in two states "active" and "inactive" to represent a toggled state.
bool ImGuiToggleButton(const char* label, bool active);

// Combo item that allows selecting a set of values.
float ImGuiFloatCombo(const char* label, float current_value, float* values, size_t value_count);

}; // namespace ws