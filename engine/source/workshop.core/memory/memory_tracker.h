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

// Describes a high level grouping of different types of allocations.
static inline const char* memory_type_names[] =
{
#define MEMORY_TYPE(name, path) path,
#include "workshop.core/memory/memory_type.inc"
#undef MEMORY_TYPE
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
    inline static string_hash k_ignore_asset = string_hash("ignore_asset");

	memory_scope(memory_type type, string_hash asset_id = string_hash::empty, string_hash fallback_asset_id = string_hash::empty);
	~memory_scope();

public:

	// Gets the current scope, or nullptr if not in a scope. This is thread-local.
	static memory_scope* get_current_scope();

	// Records allocation in this scope.
	std::unique_ptr<memory_allocation> record_alloc(size_t size);

    // Gets type of memory this scope is allocating.
    memory_type get_type();

    // Gets the id of the asset this scope is allocating memory for.
    string_hash get_asset_id();

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

#pragma pack(push, 1)
    struct raw_alloc_tag
    {
        uint16_t    magic;
        uint16_t    type;
        string_hash asset_id;
        uint32_t    size;
    };
#pragma pack(pop)

    // Size of the tag placed at the end of allocated blocks by record_raw_alloc.
    static constexpr size_t k_raw_alloc_tag_size = sizeof(raw_alloc_tag);

    // Magic number for sanity checking allocation tags.
    static constexpr uint16_t k_raw_alloc_tag_magic = 0xBEAD;

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

    // Records a raw malloc allocation by placing the memory type at the end of the allocation.
    // This shouldn't be called directly, its here for memory hooks to invoke.
    // Size of allocation should always include raw_alloc_tag_size 
    void record_raw_alloc(void* ptr, size_t size);

    // Records a raw free of an allocation previously record with record_raw_alloc.
    void record_raw_free(void* ptr);

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

	std::recursive_mutex m_asset_mutex;

	std::array<type_bucket, static_cast<int>(memory_type::COUNT)> m_types;

};

}; // namespace ws
