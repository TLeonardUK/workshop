// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/transactions/editor_transaction_modify_component.h"

#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/ecs/object_manager.h"

namespace ws {

editor_transaction_modify_component::editor_transaction_modify_component(engine& engine, editor& editor, object handle, std::type_index component_type, const std::vector<uint8_t>& before_state, const std::vector<uint8_t>& after_state)
	: m_engine(engine)
	, m_editor(editor)
	, m_handle(handle)
	, m_component_type(component_type)
	, m_before_state(before_state)
	, m_after_state(after_state)
{
}

void editor_transaction_modify_component::execute()
{
	m_engine.get_default_world().get_object_manager().deserialize_component(m_handle, m_after_state);
}

void editor_transaction_modify_component::rollback()
{
	m_engine.get_default_world().get_object_manager().deserialize_component(m_handle, m_before_state);
}

std::string editor_transaction_modify_component::get_name()
{
	return "Modify Component";
}

}; // namespace ws
