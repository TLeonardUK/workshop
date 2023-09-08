// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.editor/editor/editor_clipboard.h"
#include "workshop.engine/ecs/object.h"

namespace ws {

// ================================================================================================
//  Base class for all transactions that can be performed and reversed.
// ================================================================================================
class editor_object_clipboard_entry : public editor_clipboard_entry
{
public:
    struct object_entry
    {
        std::vector<uint8_t> serialized;
        object original_handle;
    };

public:

    editor_object_clipboard_entry(const std::vector<object_entry>& entries);

    // Gets a name that describes the entry and is used to reference this in the UI.
    virtual std::string get_name() override;

    // Returns number of objects in entry.
    size_t size();

    // Gets the serialized object at the given index.
    const object_entry& get(size_t i);

private:
    std::vector<object_entry> m_entries;

};
}; // namespace ws
