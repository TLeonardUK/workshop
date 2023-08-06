// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"

namespace ws {

render_static_mesh::render_static_mesh(render_object_id id, renderer& renderer)
    : render_object(id, &renderer, render_visibility_flags::physical)
{
    create_render_data();
}

render_static_mesh::~render_static_mesh()
{
    unregister_asset_change_callbacks();

    m_renderer->unqueue_callbacks(this);

    destroy_render_data();
}

asset_ptr<model> render_static_mesh::get_model()
{
    return m_model;
}

void render_static_mesh::set_model(const asset_ptr<model>& model)
{
    unregister_asset_change_callbacks();

    m_model = model;

    register_asset_change_callbacks();

    recreate_render_data();
}

void render_static_mesh::set_local_transform(const vector3& location, const quat& rotation, const vector3& scale)
{
    render_object::set_local_transform(location, rotation, scale);

    update_render_data();
}

void render_static_mesh::register_asset_change_callbacks()
{
    if (!m_model.is_valid())
    {
        return;
    }

    auto callback = [this]() {
        m_renderer->queue_callback(this, [this]() {
            recreate_render_data();
        });
    };

    m_model_change_callback_key = m_model.register_changed_callback(callback);

    if (m_model.is_loaded())
    {
        for (size_t i = 0; i < m_model->materials.size(); i++)
        {
            model::material_info& mat_info = m_model->materials[i];
            if (!mat_info.material.is_loaded())
            {
                continue;
            }

            auto callback_key = mat_info.material.register_changed_callback(callback);
            m_material_change_callback_keys.push_back({ mat_info.material, callback_key });
        }    
    }
}

void render_static_mesh::unregister_asset_change_callbacks()
{
    if (!m_model.is_valid())
    {
        return;
    }

    m_model.unregister_changed_callback(m_model_change_callback_key);

    for (material_callback& pair : m_material_change_callback_keys)
    {
        pair.material.unregister_changed_callback(pair.key);
    }    

    m_model_change_callback_key = 0;
    m_material_change_callback_keys.clear();
}

void render_static_mesh::recreate_render_data()
{
    destroy_render_data();
    create_render_data();

    bounds_modified();
}

void render_static_mesh::create_render_data()
{
    if (m_geometry_instance_info)
    {
        destroy_render_data();
    }
 
    m_geometry_instance_info = m_renderer->get_param_block_manager().create_param_block("geometry_instance_info");
    update_render_data();

    if (!m_model.is_loaded())
    {
        return;
    }

    // Register batch instance for each mesh.
    for (size_t i = 0; i < m_model->meshes.size(); i++)
    {
        model::mesh_info& mesh_info = m_model->meshes[i];

        model::material_info& mat_info = m_model->materials[mesh_info.material_index];
        if (!mat_info.material.is_loaded())
        {
            continue;
        }

        // Add visibility object to cull this mesh.
        obb bounds = obb(mesh_info.bounds, get_transform());

        mesh_visibility& visibility = m_mesh_visibility.emplace_back();
        visibility.mesh_index = i;
        visibility.id = m_renderer->get_visibility_manager().register_object(bounds, render_visibility_flags::physical);

        render_batch_key key;
        key.mesh_index = i;
        key.material_index = mesh_info.material_index;
        key.model = m_model;
        key.domain = mat_info.material->domain;
        key.usage = render_batch_usage::static_mesh;

        render_batch_instance info;
        info.key = key;
        info.object = this;
        info.visibility_id = visibility.id;
        info.param_block = m_geometry_instance_info.get();

        m_renderer->get_batch_manager().register_instance(info);

        m_registered_batches.push_back(info);
    }
}

void render_static_mesh::destroy_render_data()
{
    for (mesh_visibility& id : m_mesh_visibility)
    {
        m_renderer->get_visibility_manager().unregister_object(id.id);
    }
    m_mesh_visibility.clear();

    for (auto& batch_instance : m_registered_batches)
    {
        m_renderer->get_batch_manager().unregister_instance(batch_instance);
    }
    m_registered_batches.clear();

    m_geometry_instance_info = nullptr;
}

void render_static_mesh::update_render_data()
{
    m_geometry_instance_info->set("model_matrix", get_transform());
}

obb render_static_mesh::get_bounds()
{
    if (!m_model.is_loaded())
    {
        return obb(aabb::zero, get_transform());
    }
    else
    {

        return obb(m_model->geometry->bounds, get_transform());
    }
}

void render_static_mesh::bounds_modified()
{
    render_object::bounds_modified();

    for (mesh_visibility& id : m_mesh_visibility)
    {
        obb bounds = obb(m_model->meshes[id.mesh_index].bounds, get_transform());
        m_renderer->get_visibility_manager().update_object_bounds(id.id, bounds);
    }
}

}; // namespace ws
