// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/editor_undo_stack.h"

namespace ws {

void editor_undo_stack::push(std::unique_ptr<editor_transaction> transaction)
{
	transaction->execute();

	m_redo_stack.clear();
	m_undo_stack.push_back(std::move(transaction));

	if (m_undo_stack.size() > k_max_stack_size)
	{
		m_undo_stack.erase(m_undo_stack.begin());
	}
}

void editor_undo_stack::undo()
{
	if (!m_undo_stack.empty())
	{
		std::unique_ptr<editor_transaction> transaction = std::move(m_undo_stack.back());
		transaction->rollback();

		m_undo_stack.pop_back();
		m_redo_stack.push_back(std::move(transaction));

		if (m_redo_stack.size() > k_max_stack_size)
		{
			m_redo_stack.erase(m_redo_stack.begin());
		}
	}
}

void editor_undo_stack::redo()
{
	if (!m_redo_stack.empty())
	{
		std::unique_ptr<editor_transaction> transaction = std::move(m_redo_stack.back());
		transaction->execute();

		m_redo_stack.pop_back();
		m_undo_stack.push_back(std::move(transaction));

		if (m_undo_stack.size() > k_max_stack_size)
		{
			m_undo_stack.erase(m_undo_stack.begin());
		}
	}
}

void editor_undo_stack::clear()
{
	m_redo_stack.clear();
	m_undo_stack.clear();
}

std::string editor_undo_stack::get_next_undo_name()
{
	if (!m_undo_stack.empty())
	{
		return m_undo_stack.back()->get_name();
	}
	return "";
}

std::string editor_undo_stack::get_next_redo_name()
{
	if (!m_redo_stack.empty())
	{
		return m_redo_stack.back()->get_name();
	}
	return "";
}

}; // namespace ws
