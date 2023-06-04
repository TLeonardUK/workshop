// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"

namespace ws {

render_static_mesh::render_static_mesh(renderer& renderer)
    : m_renderer(renderer)
{
    create_render_data();
}

render_static_mesh::~render_static_mesh()
{
    unregister_asset_change_callbacks();

    m_renderer.unqueue_callbacks(this);

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
        m_renderer.queue_callback(this, [this]() {
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
}

void render_static_mesh::create_render_data()
{
    if (m_geometry_instance_info)
    {
        destroy_render_data();
    }
 
    m_geometry_instance_info = m_renderer.get_param_block_manager().create_param_block("geometry_instance_info");
    update_render_data();

    if (!m_model.is_loaded())
    {
        return;
    }

    // Register batch instance for each material.
    for (size_t i = 0; i < m_model->materials.size(); i++)
    {
        model::material_info& mat_info = m_model->materials[i];
        if (!mat_info.material.is_loaded())
        {
            continue;
        }

        render_batch_key key;
        key.material_index = i;
        key.model = m_model;
        key.domain = mat_info.material->domain;
        key.usage = render_batch_usage::static_mesh;

        render_batch_instance info;
        info.key = key;
        info.object = this;
        info.param_block = m_geometry_instance_info.get();

        m_renderer.get_batch_manager().register_instance(info);

        m_registered_batches.push_back(info);
    }
}

void render_static_mesh::destroy_render_data()
{
    for (auto& batch_instance : m_registered_batches)
    {
        m_renderer.get_batch_manager().unregister_instance(batch_instance);
    }
    m_registered_batches.clear();

    m_geometry_instance_info = nullptr;
}

void render_static_mesh::update_render_data()
{
    matrix4 transform = matrix4::scale(m_local_scale) * 
                        matrix4::rotation(m_local_rotation) * 
                        matrix4::translate(m_local_location);

    m_geometry_instance_info->set("model_matrix", transform);
}

}; // namespace ws
