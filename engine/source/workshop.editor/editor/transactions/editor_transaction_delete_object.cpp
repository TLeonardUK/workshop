// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/transactions/editor_transaction_delete_object.h"

#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

editor_transaction_delete_object::editor_transaction_delete_object(engine& engine, editor& editor, object handle)
	: m_engine(engine)
	, m_editor(editor)
	, m_handle(handle)
{
	m_serialized = m_engine.get_default_world().get_object_manager().serialize_object(m_handle);
}

void editor_transaction_delete_object::execute()
{
	m_engine.get_default_world().get_object_manager().destroy_object(m_handle);
}

void editor_transaction_delete_object::rollback()
{
	auto& object_manager = m_engine.get_default_world().get_object_manager();

	object_manager.create_object("untitled", m_handle);
	object_manager.deserialize_object(m_handle, m_serialized);
}

std::string editor_transaction_delete_object::get_name()
{
	return "Delete Object";
}

}; // namespace ws
