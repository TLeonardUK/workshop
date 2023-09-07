// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/editor_window.h"

namespace ws {

void editor_window::open()
{
    m_open = true;
}

void editor_window::close()
{
    m_open = false;
}

bool editor_window::is_open()
{
    return m_open;
}

}; // namespace ws
