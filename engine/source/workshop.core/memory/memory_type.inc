// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================

// Format is:
//		Name, Path

// ================================================================================================
//  Generic
// ================================================================================================

MEMORY_TYPE(memory_tracking__untagged,	            "memory tracking/untagged")
MEMORY_TYPE(memory_tracking__overhead,	            "memory tracking/overhead")
MEMORY_TYPE(memory_tracking__waste, 	            "memory tracking/waste")

// Used for debugging the memory tracking system, shouldn't normally have any value.
MEMORY_TYPE(memory_tracking__debug, 	            "memory tracking/debug")

// This is useful as a high-level generic container for misc allocations of assets/etc which will
// be broken down further in lower memory scopes.
MEMORY_TYPE(asset,				                    "asset")

// ================================================================================================
//  Engine
// ================================================================================================

MEMORY_TYPE(low_level__misc_sparse_vector,	        "low level/misc sparse vector")

MEMORY_TYPE(engine__ecs,	                        "engine/ecs")
MEMORY_TYPE(engine__task_scheduler,	                "engine/task scheduler")
MEMORY_TYPE(engine__command_queue,	                "engine/command queue")

// ================================================================================================
//  Rendering
// ================================================================================================

MEMORY_TYPE(rendering__vram__vertex_buffer,		    "rendering/vram/vertex buffer")
MEMORY_TYPE(rendering__vram__index_buffer,		    "rendering/vram/index buffer")
MEMORY_TYPE(rendering__vram__raytracing_buffer,		"rendering/vram/raytracing buffer")
MEMORY_TYPE(rendering__vram__generic_buffer,	    "rendering/vram/generic buffer")
MEMORY_TYPE(rendering__vram__texture,			    "rendering/vram/texture")
MEMORY_TYPE(rendering__vram__render_target_color,	"rendering/vram/render target (color)")
MEMORY_TYPE(rendering__vram__render_target_depth,	"rendering/vram/render target (depth)")
MEMORY_TYPE(rendering__vram__descriptor_heap,	    "rendering/vram/descriptor heap")
MEMORY_TYPE(rendering__vram__param_blocks,	        "rendering/vram/param blocks")
MEMORY_TYPE(rendering__vram__queries,	            "rendering/vram/queries")
MEMORY_TYPE(rendering__vram__upload_heap,	        "rendering/vram/upload heap")
MEMORY_TYPE(rendering__vram__swapchain,	            "rendering/vram/swapchain")

MEMORY_TYPE(rendering__systems__imgui,	            "rendering/systems/imgui")

MEMORY_TYPE(rendering__descriptor_heap,	            "rendering/descriptor heap")
MEMORY_TYPE(rendering__upload_heap,	                "rendering/upload heap")
MEMORY_TYPE(rendering__light_probe_grid,	        "rendering/light probe grid")