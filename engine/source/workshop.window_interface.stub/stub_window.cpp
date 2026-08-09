// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.window_interface.stub/stub_window.h"
#include "workshop.window_interface.stub/stub_window_interface.h"

namespace ws {

stub_window::stub_window(stub_window_interface* owner)
    : m_owner(owner)
{
}

stub_window::~stub_window() = default;

result<void> stub_window::apply_changes()
{
    return true;
}

void* stub_window::get_platform_handle()
{
    return nullptr;
}

}; // namespace ws
