// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/transactions/editor_transaction_delete_objects.h"

#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

editor_transaction_delete_objects::editor_transaction_delete_objects(engine& engine, editor& editor, const std::vector<object>& handles)
	: m_engine(engine)
	, m_editor(editor)
{
	for (object handle : handles)
	{
		state& new_state = m_states.emplace_back();
		new_state.handle = handle;
		new_state.serialized = m_engine.get_default_world().get_object_manager().serialize_object(new_state.handle);
	}
}

void editor_transaction_delete_objects::execute()
{
	auto& object_manager = m_engine.get_default_world().get_object_manager();

	for (auto iter = m_states.begin(); iter != m_states.end(); iter++)
	{
		state& s = *iter;

		object_manager.destroy_object(s.handle);
	}
}

void editor_transaction_delete_objects::rollback()
{
	auto& object_manager = m_engine.get_default_world().get_object_manager();

	for (auto iter = m_states.rbegin(); iter != m_states.rend(); iter++)
	{
		state& s = *iter;
	
		object_manager.create_object("untitled", s.handle);
		object_manager.deserialize_object(s.handle, s.serialized, false);
	}

	for (auto iter = m_states.rbegin(); iter != m_states.rend(); iter++)
	{
		state& s = *iter;
		object_manager.object_edited(s.handle, component_modification_source::serialization);
	}
}

std::string editor_transaction_delete_objects::get_name()
{
	return "Delete Objects";
}

}; // namespace ws
