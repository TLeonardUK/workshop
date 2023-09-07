// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/transactions/editor_transaction_create_object.h"

#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

editor_transaction_create_object::editor_transaction_create_object(engine& engine, editor& editor, object handle)
	: m_engine(engine)
	, m_editor(editor)
	, m_handle(handle)
	, m_alive(true)
{
	m_serialized = m_engine.get_default_world().get_object_manager().serialize_object(m_handle);	
}

void editor_transaction_create_object::execute()
{
	if (!m_alive)
	{
		auto& object_manager = m_engine.get_default_world().get_object_manager();

		object_manager.create_object("untitled", m_handle);
		object_manager.deserialize_object(m_handle, m_serialized);

		m_alive = true;
	}
}

void editor_transaction_create_object::rollback()
{
	if (m_alive)
	{
		m_engine.get_default_world().get_object_manager().destroy_object(m_handle);
		m_alive = false;
	}
}

std::string editor_transaction_create_object::get_name()
{
	return "Create Object";
}

}; // namespace ws
