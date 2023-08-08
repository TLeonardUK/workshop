// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================

// Format is:
//		Name, Path

// ================================================================================================
//  Generic
// ================================================================================================

// This is useful as a high-level generic container for misc allocations of assets/etc which will
// be broken down further in lower memory scopes.
MEMORY_TYPE(asset,				"asset")

// ================================================================================================
//  Rendering
// ================================================================================================

MEMORY_TYPE(rendering__vram__vertex_buffer,		"rendering/vram/vertex buffer")
MEMORY_TYPE(rendering__vram__index_buffer,		"rendering/vram/index buffer")
MEMORY_TYPE(rendering__vram__generic_buffer,	"rendering/vram/generic buffer")
MEMORY_TYPE(rendering__vram__texture,			"rendering/vram/texture")
MEMORY_TYPE(rendering__vram__render_target,		"rendering/vram/render target")