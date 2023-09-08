// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ws {

// ================================================================================================
//  Base class for all transactions that can be performed and reversed.
// ================================================================================================
class editor_clipboard_entry
{
public:

    // Gets a name that describes the entry and is used to reference this in the UI.
    virtual std::string get_name() = 0;

};

// ================================================================================================
//  Handles rolling trasactions forwards and backwards to support undo/redo in the editor.
// ================================================================================================
class editor_clipboard
{
public:

    // Sets the current contents of the clipboard.
    void set(std::unique_ptr<editor_clipboard_entry> entry);

    // Returns the current entry of the clipboard without removing it.
    editor_clipboard_entry* peak();

    // Returns the current entry of the clipboard and removes it.
    std::unique_ptr<editor_clipboard_entry> remove();

    // Returns true if there is nothing currently in the clipboard.
    bool empty();

    // Returns true if the clipboard contains an entry of the given type.
    template <typename entry_type>
    bool contains()
    {
        return (m_entry && dynamic_cast<entry_type*>(m_entry.get()) != nullptr);
    }

protected:    
    std::unique_ptr<editor_clipboard_entry> m_entry;

};

}; // namespace ws
