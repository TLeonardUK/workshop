// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/memory/memory_tracker.h"

#include <Windows.h>

namespace ws {
namespace {

static thread_local memory_scope* g_tls_memory_scope = nullptr;

};

memory_allocation::memory_allocation(memory_type type, string_hash asset_id, size_t size)
	: m_type(type)
	, m_asset_id(asset_id)
	, m_size(size)
{
}

memory_allocation::~memory_allocation()
{
	if (m_type != memory_type::COUNT)
	{
		memory_tracker* tracker = memory_tracker::try_get();
		if (tracker)
		{
			tracker->record_free(m_type, m_asset_id, m_size);
		}
	}
}

memory_scope::memory_scope(memory_type type, string_hash asset_id, string_hash fallback_asset_id)
	: m_type(type)
	, m_asset_id(asset_id)
{ 
	m_old_memory_scope = g_tls_memory_scope;
	g_tls_memory_scope = this;

	// If asset_id = nullptr, we inhert whatever is further up in the stack.
    if (m_asset_id == k_ignore_asset)
    {
        m_asset_id = string_hash::empty;
    }
	else if (m_asset_id == string_hash::empty)
	{
		memory_scope* scope = g_tls_memory_scope;
		while (scope)
		{
			if (scope->m_asset_id != string_hash::empty)
			{
				m_asset_id = scope->m_asset_id;
				break;
			}
			scope = scope->m_old_memory_scope;
		}

		if (m_asset_id == string_hash::empty)
		{
			m_asset_id = fallback_asset_id;
		}
	}
}

memory_scope::~memory_scope()
{
	g_tls_memory_scope = m_old_memory_scope;
}

memory_scope* memory_scope::get_current_scope()
{
	return g_tls_memory_scope;
}

std::unique_ptr<memory_allocation> memory_scope::record_alloc(size_t size)
{
	memory_tracker* tracker = memory_tracker::try_get();
	if (tracker)
	{
		tracker->record_alloc(m_type, m_asset_id, size);

		return std::make_unique<memory_allocation>(m_type, m_asset_id, size);
	}

	return nullptr;
}

memory_type memory_scope::get_type()
{
    return m_type;
}

string_hash memory_scope::get_asset_id()
{
    return m_asset_id;
}

size_t memory_tracker::get_memory_allocation_count(memory_type type)
{
	type_bucket& bucket = m_types[static_cast<int>(type)];
	return bucket.allocation_count.load();
}

size_t memory_tracker::get_memory_used_bytes(memory_type type)
{
	type_bucket& bucket = m_types[static_cast<int>(type)];
	return bucket.allocation_bytes.load();
}

std::vector<memory_tracker::asset_state> memory_tracker::get_assets(memory_type type)
{
	type_bucket& bucket = m_types[static_cast<int>(type)];

	std::scoped_lock lock(m_asset_mutex);
	std::vector<memory_tracker::asset_state> result;

	for (auto& pair : bucket.assets)
	{
		memory_tracker::asset_state& state = result.emplace_back();
		state.id = pair.first;
		state.allocation_count = pair.second.allocation_count.load();
		state.used_bytes = pair.second.allocation_bytes.load();
	}

	return result;
}

std::unordered_map<string_hash, memory_tracker::asset_breakdown> memory_tracker::get_asset_breakdown()
{
	std::unordered_map<string_hash, asset_breakdown> result;

	std::scoped_lock lock(m_asset_mutex);

	for (size_t i = 0; i < (size_t)memory_type::COUNT; i++)
	{
		memory_type type = static_cast<memory_type>(memory_type::COUNT);
		type_bucket& bucket = m_types[i];

		for (auto& [id, state] : bucket.assets)
		{
			asset_breakdown& breakdown = result[id];
			breakdown.aggregate.id = id;
			breakdown.aggregate.allocation_count += state.allocation_count;
			breakdown.aggregate.used_bytes += state.allocation_bytes;

			asset_state& type_state = breakdown.by_type[type];
			type_state.allocation_count += state.allocation_count;
			type_state.used_bytes += state.allocation_bytes;
		}
	}

	return result;
}

void memory_tracker::record_alloc(memory_type type, string_hash asset_id, size_t size)
{
	type_bucket& bucket = m_types[static_cast<int>(type)];

    if (size == 0)
    {
        return;
    }

    if (type == memory_type::engine__command_queue && asset_id != string_hash::empty)
    {
        __debugbreak();
    }

	bucket.allocation_count.fetch_add(1);
	bucket.allocation_bytes.fetch_add(size);

	if (asset_id != string_hash::empty)
	{
		std::scoped_lock lock(m_asset_mutex);

		asset_bucket& asset_bucket = bucket.assets[asset_id];
		asset_bucket.allocation_count.fetch_add(1);
		asset_bucket.allocation_bytes.fetch_add(size);
	}
}

void memory_tracker::record_free(memory_type type, string_hash asset_id, size_t size)
{
	type_bucket& bucket = m_types[static_cast<int>(type)];

    if (size == 0)
    {
        return;
    }

	bucket.allocation_count.fetch_sub(1);
	bucket.allocation_bytes.fetch_sub(size);

	if (asset_id != string_hash::empty)
	{
		std::scoped_lock lock(m_asset_mutex);

		auto iter = bucket.assets.find(asset_id);

		asset_bucket& asset_bucket = iter->second;
		asset_bucket.allocation_bytes.fetch_sub(size);

		size_t result = asset_bucket.allocation_count.fetch_sub(1);
		if (result == 1)
		{
			bucket.assets.erase(iter);
		}
	}
}

#pragma optimize("", off)

void memory_tracker::record_raw_alloc(void* ptr, size_t size)
{
    memory_scope* scope = memory_scope::get_current_scope();

    size_t buffer_size = _msize(ptr);
    raw_alloc_tag* tag = reinterpret_cast<raw_alloc_tag*>((uint8_t*)ptr + buffer_size - sizeof(raw_alloc_tag));
    tag->magic = k_raw_alloc_tag_magic;
    tag->type = (decltype(tag->type))(scope ? scope->get_type() : memory_type::memory_tracking__untagged);
    tag->asset_id = scope ? scope->get_asset_id() : string_hash::empty;
    tag->size = (uint32_t)size;

    record_alloc((memory_type)tag->type, tag->asset_id, tag->size - sizeof(raw_alloc_tag));
    record_alloc(memory_type::memory_tracking__overhead, string_hash::empty, sizeof(raw_alloc_tag));
    record_alloc(memory_type::memory_tracking__waste, string_hash::empty, buffer_size - tag->size);
}

void memory_tracker::record_raw_free(void* ptr)
{
    size_t buffer_size = _msize(ptr);
    if (buffer_size < sizeof(raw_alloc_tag))
    {
        return;
    }

    raw_alloc_tag* tag = reinterpret_cast<raw_alloc_tag*>((uint8_t*)ptr + buffer_size - sizeof(raw_alloc_tag));
    if (tag->magic != k_raw_alloc_tag_magic)
    {
        return;
    }

    record_free((memory_type)tag->type, tag->asset_id, tag->size - sizeof(raw_alloc_tag));
    record_free(memory_type::memory_tracking__overhead, string_hash::empty, sizeof(raw_alloc_tag));
    record_free(memory_type::memory_tracking__waste, string_hash::empty, buffer_size - tag->size);
}

}; // namespace workshop
