// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/assets/shader/shader.h"

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

asset_ptr<material> render_static_mesh::get_material(size_t index)
{
    if (m_override_materials.size() > index)
    {
        return m_override_materials[index];
    }

    if (m_model.is_loaded())
    {
        if (m_model->materials.size() > index)
        {
            return m_model->materials[index].material;
        }
    }

    return {};
}

void render_static_mesh::set_materials(const std::vector<asset_ptr<material>>& materials)
{
    unregister_asset_change_callbacks();

    m_override_materials = materials;

    register_asset_change_callbacks();

    recreate_render_data();
}

void render_static_mesh::set_local_transform(const vector3& location, const quat& rotation, const vector3& scale)
{
    render_object::set_local_transform(location, rotation, scale);

    update_render_data();
}

void render_static_mesh::set_render_gpu_flags(render_gpu_flags flags)
{
    render_object::set_render_gpu_flags(flags);

    update_render_data();
}

void render_static_mesh::set_draw_flags(render_draw_flags flags)
{
    render_object::set_draw_flags(flags);

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

            auto callback_key = mat_info.material.register_changed_callback(callback);
            m_material_change_callback_keys.push_back({ mat_info.material, callback_key });
        }    
    }

    for (size_t i = 0; i < m_override_materials.size(); i++)
    {
        auto& mat = m_override_materials[i];
        if (mat.is_valid())
        {
            auto callback_key = mat.register_changed_callback(callback);
            m_material_change_callback_keys.push_back({ mat, callback_key });
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

void render_static_mesh::set_visibility(bool visible)
{
    render_object::set_visibility(visible);

    for (mesh_visibility& visibility : m_mesh_visibility)
    {
        m_renderer->get_visibility_manager().set_object_manual_visibility(visibility.id, visible);
    }
}

render_visibility_manager::object_id render_static_mesh::get_submesh_visibility_id(size_t submesh_index)
{
    for (mesh_visibility& v : m_mesh_visibility)
    {
        if (v.mesh_index == submesh_index)
        {
            return v.id;
        }
    }

    return {};
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
        
        asset_ptr<material> mat = mat_info.material;
        if (mesh_info.material_index < m_override_materials.size())
        {
            asset_ptr<material>& override_mat = m_override_materials[mesh_info.material_index];
            if (override_mat.is_valid())
            {
                mat = override_mat;
            }
        }

        if (!mat.is_loaded())
        {
            continue;
        }

        // Add visibility object to cull this mesh.
        obb bounds = obb(mesh_info.bounds, get_transform());

        mesh_visibility& visibility = m_mesh_visibility.emplace_back();
        visibility.mesh_index = i;
        visibility.id = m_renderer->get_visibility_manager().register_object(bounds, m_world_id, render_visibility_flags::physical);

        render_batch_key key;
        key.mesh_index = i;
        key.material = mat;
        key.model = m_model;
        key.domain = mat->domain;
        key.usage = render_batch_usage::static_mesh;

        render_batch_instance info;
        info.key = key;
        info.object = this;
        info.visibility_id = visibility.id;
        info.param_block = m_geometry_instance_info.get();

        m_renderer->get_batch_manager().register_instance(info);

        m_registered_batches.push_back(info);

        // If we support raytracing create add an entry for this mesh into the scene tlas.
        if (m_renderer->get_render_interface().check_feature(ri_feature::raytracing))
        {
            // Create metadata for the blas
            ri_raytracing_blas* blas = m_model->find_or_create_blas(i);

            tlas_instance* tlas = &m_registered_tlas_instances.emplace_back();
            tlas->metadata = m_renderer->get_param_block_manager().create_param_block("tlas_metadata");

            size_t table_index, table_offset;
            m_model->get_model_info_param_block(i).get_table(table_index, table_offset);
            tlas->metadata->set("model_info_table", (uint32_t)table_index);
            tlas->metadata->set("model_info_offset", (uint32_t)table_offset);
            mat->get_material_info_param_block()->get_table(table_index, table_offset);
            tlas->metadata->set("material_info_table", (uint32_t)table_index);
            tlas->metadata->set("material_info_offset", (uint32_t)table_offset);
            tlas->metadata->set("gpu_flags", (uint32_t)m_gpu_flags);

            bool is_transparent = (mat->domain == material_domain::transparent || mat->domain == material_domain::masked);

            tlas->mask = ray_mask::normal;
            if (mat->domain == material_domain::sky)
            {
                tlas->mask = ray_mask::sky;
            }

            m_visible_in_rt = has_draw_flag(render_draw_flags::geometry);

            tlas->id = m_renderer->get_scene_tlas().add_instance(blas, get_transform(), (size_t)mat->domain, !is_transparent, tlas->metadata.get(), m_visible_in_rt ? (uint32_t)tlas->mask : (uint32_t)ray_mask::invisible);
        }
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

    for (tlas_instance& instance : m_registered_tlas_instances)
    {
        m_renderer->get_scene_tlas().remove_instance(instance.id);
    }
    m_registered_tlas_instances.clear();

    m_geometry_instance_info = nullptr;
}

void render_static_mesh::update_render_data()
{
    matrix4 transform = get_transform();

    bool changed = false;
    changed |= m_geometry_instance_info->set("model_matrix", transform);
    changed |= m_geometry_instance_info->set("gpu_flags", (uint32_t)m_gpu_flags);

    // Mark as changed if draw in geometry pass.
    bool visible_in_rt = has_draw_flag(render_draw_flags::geometry);
    changed |= (visible_in_rt != m_visible_in_rt);
    m_visible_in_rt = visible_in_rt;

    // Update tlas transform and metadata.
    if (changed)
    {
        for (tlas_instance& instance : m_registered_tlas_instances)
        {
            instance.metadata->set("gpu_flags", (uint32_t)m_gpu_flags);
            m_renderer->get_scene_tlas().update_instance(instance.id, transform, m_visible_in_rt ? (uint32_t)instance.mask : (uint32_t)ray_mask::invisible);
        }
    }
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
        m_renderer->get_visibility_manager().update_object_bounds(id.id, m_world_id, bounds);
    }
}

}; // namespace ws
