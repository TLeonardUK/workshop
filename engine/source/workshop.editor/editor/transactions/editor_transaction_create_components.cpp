// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/transactions/editor_transaction_create_components.h"

#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

editor_transaction_create_components::editor_transaction_create_components(engine& engine, editor& editor, object handle, const std::vector<std::type_index>& component_types)
	: m_engine(engine)
	, m_editor(editor)
	, m_handle(handle)
	, m_alive(true)
{
	for (const std::type_index& index : component_types)
	{
		component_info& info = m_components.emplace_back();
		info.type = index;
		info.serialized = m_engine.get_default_world().get_object_manager().serialize_component(m_handle, index);
	}
}

void editor_transaction_create_components::execute()
{
	if (!m_alive)
	{
		auto& object_manager = m_engine.get_default_world().get_object_manager();

		for (component_info& info : m_components)
		{
			object_manager.deserialize_component(m_handle, info.serialized);
		}

		m_alive = true;
	}
}

void editor_transaction_create_components::rollback()
{
	if (m_alive)
	{
		auto& object_manager = m_engine.get_default_world().get_object_manager();

		for (component_info& info : m_components)
		{
			object_manager.remove_component(m_handle, info.type);
		}

		m_alive = false;
	}
}

std::string editor_transaction_create_components::get_name()
{
	return "Create Components";
}

}; // namespace ws
