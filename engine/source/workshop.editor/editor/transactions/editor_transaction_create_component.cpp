// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/transactions/editor_transaction_create_component.h"

#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

editor_transaction_create_component::editor_transaction_create_component(engine& engine, editor& editor, object handle, std::type_index component_type)
	: m_engine(engine)
	, m_editor(editor)
	, m_handle(handle)
	, m_component_type(component_type)
	, m_alive(true)
{
	m_serialized = m_engine.get_default_world().get_object_manager().serialize_component(m_handle, component_type);	
}

void editor_transaction_create_component::execute()
{
	if (!m_alive)
	{
		auto& object_manager = m_engine.get_default_world().get_object_manager();

		object_manager.deserialize_component(m_handle, m_serialized);

		m_alive = true;
	}
}

void editor_transaction_create_component::rollback()
{
	if (m_alive)
	{
		m_engine.get_default_world().get_object_manager().remove_component(m_handle, m_component_type);
		m_alive = false;
	}
}

std::string editor_transaction_create_component::get_name()
{
	return "Create Component";
}

}; // namespace ws