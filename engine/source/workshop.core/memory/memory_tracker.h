// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/singleton.h"
#include "workshop.core/hashing/string_hash.h"

#include <array>
#include <unordered_map>

namespace ws {

// Describes a high level grouping of different types of allocations.
enum class memory_type
{
#define MEMORY_TYPE(name, path) name,
#include "workshop.core/memory/memory_type.inc"
#undef MEMORY_TYPE

	COUNT
};

// Acts as an RAII container for a recorded allocations. When disposed destructed
// this automatically decreases the count.
class memory_allocation
{
public:
	memory_allocation(memory_type type, string_hash asset_id, size_t size);
	~memory_allocation();

private:
	memory_type m_type;
	string_hash m_asset_id;
	size_t m_size;

};

// Acts as RAII scope for all memory allocations. Allocations inside this scope
// will be marked with the attributes provided in its constructor.
class memory_scope
{
public:
	memory_scope(memory_type type, string_hash asset_id = string_hash::empty, string_hash fallback_asset_id = string_hash::empty);
	~memory_scope();

public:

	// Gets the current scope, or nullptr if not in a scope. This is thread-local.
	static memory_scope* get_current_scope();

	// Records allocation in this scope.
	std::unique_ptr<memory_allocation> record_alloc(size_t size);

private:
	memory_type m_type;
	string_hash m_asset_id;

	memory_scope* m_old_memory_scope = nullptr;

};

// This class is responsible for lightweight tracking of memory allocations in buckets such 
// that we can easily see where our memory is being used.
class memory_tracker
	: public singleton<memory_tracker>
{
public:

	// State of a given asset, as provided by get_assets.
	struct asset_state
	{
		string_hash id;

		size_t allocation_count;
		size_t used_bytes;
	};

	// State of a given asset, broken down by type.
	struct asset_breakdown
	{
		asset_state aggregate;
		std::unordered_map<memory_type, asset_state> by_type;
	};

	// Gets number of allocations for the given type.
	size_t get_memory_allocation_count(memory_type type);

	// Gets the number of bytes currently active for the given type.
	size_t get_memory_used_bytes(memory_type type);

	// Gets all the assets currently active for the given memory type.
	std::vector<asset_state> get_assets(memory_type type);

	// Gets all assets with their memory usage broken down by type.
	std::unordered_map<string_hash, asset_breakdown> get_asset_breakdown();

private:

	friend class memory_scope;
	friend class memory_allocation;

	// Records an allocation.
	// This shouldn't be called directly, it should come from a memory scope.
	void record_alloc(memory_type type, string_hash asset_id, size_t size);

	// Records an free.
	// This shouldn't be called directly, it should come from a memory scope.
	void record_free(memory_type type, string_hash asset_id, size_t size);

private:

	struct asset_bucket
	{
		std::atomic_size_t allocation_count = 0;
		std::atomic_size_t allocation_bytes = 0;
	};

	struct type_bucket
	{
		std::atomic_size_t allocation_count = 0;
		std::atomic_size_t allocation_bytes = 0;
		std::unordered_map<string_hash, asset_bucket> assets;
	};

	std::mutex m_asset_mutex;

	std::array<type_bucket, static_cast<int>(memory_type::COUNT)> m_types;

};

}; // namespace ws



//memory_scope scope(memory_type::vertex_buffer, "assets/models/something.yaml");
//scope.record_alloc(12024); 
//scope.record_free(1024);