// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.renderer/assets/material/material.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/common_types.h"

#include "workshop.render_interface/ri_interface.h"

#include "workshop.core/hashing/hash.h"

namespace ws {

render_batch::render_batch(render_batch_key key, renderer& render)
    : m_key(key)
    , m_renderer(render)
{
    m_resource_cache = std::make_unique<render_resource_cache>(render);
}

render_resource_cache& render_batch::get_resource_cache()
{
    return *m_resource_cache;
}

render_batch_key render_batch::get_key()
{
    return m_key;
}

const std::vector<render_batch_instance>& render_batch::get_instances()
{
    return m_instances;
}

void render_batch::clear()
{
    m_instances.clear();
}

void render_batch::add_instance(const render_batch_instance& instance)
{
    m_instances.push_back(instance);
}

void render_batch::remove_instance(const render_batch_instance& instance)
{
    auto iter = std::find_if(m_instances.begin(), m_instances.end(), [&instance](const render_batch_instance& other) {
        return other.object == instance.object;
    });

    if (iter != m_instances.end())
    {
        m_instances.erase(iter);
    }
}

render_batch_manager::render_batch_manager(renderer& render)
    : m_renderer(render)
{
}

void render_batch_manager::register_init(init_list& list)
{
}

render_batch* render_batch_manager::find_or_create_batch(render_batch_key key)
{
    if (auto iter = m_batches.find(key); iter != m_batches.end())
    {
        return iter->second.get();
    }
    else
    {
        std::unique_ptr<render_batch> batch = std::make_unique<render_batch>(key, m_renderer);

        render_batch* batch_ptr = batch.get();
        m_batches[key] = std::move(batch);

        return batch_ptr;
    }
}

void render_batch_manager::begin_frame()
{
}

void render_batch_manager::register_instance(const render_batch_instance& instance)
{
    render_batch* batch = find_or_create_batch(instance.key);
    batch->add_instance(instance);
}

void render_batch_manager::unregister_instance(const render_batch_instance& instance)
{
    render_batch* batch = find_or_create_batch(instance.key);
    batch->remove_instance(instance);
}

std::vector<render_batch*> render_batch_manager::get_batches(material_domain domain, render_batch_usage usage)
{
    std::vector<render_batch*> result;

    for (auto& [key, batch] : m_batches)
    {
        render_batch_key key = batch->get_key();
        if (key.domain == domain && key.usage == usage)
        {
            result.push_back(batch.get());
        }
    }

    return result;
}

void render_batch_manager::clear_cached_material_data(material* material)
{
    for (auto& [key, batch] : m_batches)
    {
        render_batch_key key = batch->get_key();
        if (key.domain != material->domain)
        {
            continue;
        }

        if (!key.model.is_loaded())
        {
            continue;
        }

        model::material_info& material_info = key.model->materials[key.material_index];
        if (!material_info.material.is_loaded())
        {
            continue;
        }

        if (material_info.material.get() == material)
        {
            batch->get_resource_cache().clear();
        }
    }
}

}; // namespace ws
