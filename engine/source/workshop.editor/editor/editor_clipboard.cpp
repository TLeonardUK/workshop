// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/editor_clipboard.h"

namespace ws {

void editor_clipboard::set(std::unique_ptr<editor_clipboard_entry> entry)
{
	m_entry = std::move(entry);
}

editor_clipboard_entry* editor_clipboard::peak()
{
	if (m_entry)
	{
		return m_entry.get();
	}
	return nullptr;
}

std::unique_ptr<editor_clipboard_entry> editor_clipboard::remove()
{
	std::unique_ptr<editor_clipboard_entry> result = std::move(m_entry);
	m_entry = nullptr;

	return std::move(result);
}

bool editor_clipboard::empty()
{
	return m_entry == nullptr;
}

}; // namespace ws
