// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/transactions/editor_transaction_change_selected_objects.h"

namespace ws {

editor_transaction_change_selected_objects::editor_transaction_change_selected_objects(editor& editor, const std::vector<object>& new_objects)
	: m_editor(editor)
	, m_new(new_objects)
{
	m_previous = m_editor.get_selected_objects();
}

void editor_transaction_change_selected_objects::execute()
{
	m_editor.set_selected_objects_untransacted(m_new);
}

void editor_transaction_change_selected_objects::rollback()
{
	m_editor.set_selected_objects_untransacted(m_previous);
}

std::string editor_transaction_change_selected_objects::get_name()
{
	return "Change Selected Objects";
}

}; // namespace ws
