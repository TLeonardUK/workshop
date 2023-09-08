// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/clipboard/editor_object_clipboard_entry.h"

namespace ws {

editor_object_clipboard_entry::editor_object_clipboard_entry(const std::vector<object_entry>& entries)
	: m_entries(entries)
{
}

std::string editor_object_clipboard_entry::get_name()
{
	return "Objects";
}

size_t editor_object_clipboard_entry::size()
{
	return m_entries.size();
}

const editor_object_clipboard_entry::object_entry& editor_object_clipboard_entry::get(size_t i)
{
	return m_entries[i];
}

}; // namespace ws
