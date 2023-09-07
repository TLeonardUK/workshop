// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.editor/editor/transactions/editor_transaction_change_object_transform.h"

#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"

#include "workshop.engine/ecs/object_manager.h"

#include "workshop.game_framework/systems/transform/transform_system.h"

namespace ws {

editor_transaction_change_object_transform::editor_transaction_change_object_transform(engine& engine, editor& editor)
	: m_editor(editor)
	, m_engine(engine)
{
}

void editor_transaction_change_object_transform::add_object(object obj,
	const vector3& previous_world_location, const quat& previous_world_rotation, const vector3& previous_world_scale,
	const vector3& new_world_location, const quat& new_world_rotation, const vector3& new_world_scale
)
{
	m_previous.push_back({ obj, previous_world_location, previous_world_rotation, previous_world_scale });
	m_new.push_back({ obj, new_world_location, new_world_rotation, new_world_scale });
}

void editor_transaction_change_object_transform::execute()
{
	object_manager& obj_manager = m_engine.get_default_world().get_object_manager();
	transform_system* transform_sys = obj_manager.get_system<transform_system>();

	for (object_state& state : m_new)
	{
		transform_sys->set_world_transform(state.obj, state.world_location, state.world_rotation, state.world_scale);
	}
}

void editor_transaction_change_object_transform::rollback()
{
	object_manager& obj_manager = m_engine.get_default_world().get_object_manager();
	transform_system* transform_sys = obj_manager.get_system<transform_system>();

	for (object_state& state : m_previous)
	{
		transform_sys->set_world_transform(state.obj, state.world_location, state.world_rotation, state.world_scale);
	}
}

std::string editor_transaction_change_object_transform::get_name()
{
	return "Change Object Transform";
}

}; // namespace ws
