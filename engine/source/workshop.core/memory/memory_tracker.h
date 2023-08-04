// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/singleton.h"

namespace ws {

// This class is responsible for lightweight tracking of memory allocations in buckets such 
// that we can easily see where our memory is being used.
class memory_tracker
	: public singleton<memory_tracker>
{
public:

public:

	// TODO

//	void record_alloc(memory_bucket bucket, size_t size);

	// Records memory of the given size being freed.
//	void record_free(memory_bucket bucket, size_t size);

};

}; // namespace ws
