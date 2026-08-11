// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_raytracing_blas.h"
#include "workshop.core/utils/result.h"
#include "workshop.core/utils/event.h"
#include "workshop.render_interface.vulkan/vulkan_headers.h"

#include <memory>
#include <string>

namespace ws {

class vulkan_render_interface;
class vulkan_ri_command_list;

// ================================================================================================
//  Implementation of a bottom level acceleration structure for Vulkan.
// ================================================================================================
class vulkan_ri_raytracing_blas : public ri_raytracing_blas
{
public:
    vulkan_ri_raytracing_blas(vulkan_render_interface& renderer, const char* debug_name);
    virtual ~vulkan_ri_raytracing_blas();

    // Creates the vulkan resources required by this structure.
    result<void> create_resources();
    void destroy_resources();

    virtual void update(ri_buffer* vertex_buffer, ri_buffer* index_buffer) override;

    // Called by interface each frame if building is required.
    void build(vulkan_ri_command_list& cmd_list);

    // Returns true if this structure is pending a compaction pass.
    bool is_pending_compaction();

    // Called when read to compact structure.
    bool can_compact();

    // Compacts the structure.
    void compact(vulkan_ri_command_list& cmd_list);

    VkDeviceAddress get_gpu_address();

    const ri_buffer& get_as_buffer() const;

    // Invoked whenever this blas is modified, hooked by tlas to know when it needs to update.
    event<> on_modified;

private:
    void mark_dirty();

    VkAccelerationStructureBuildGeometryInfoKHR get_input_desc();

private:
    vulkan_render_interface& m_renderer;
    std::string m_debug_name;

    std::unique_ptr<ri_buffer> m_scratch;
    std::unique_ptr<ri_buffer> m_resource;

    VkAccelerationStructureKHR m_acceleration_structure = VK_NULL_HANDLE;
    VkQueryPool m_compaction_query_pool = VK_NULL_HANDLE;

    ri_buffer* m_build_vertex_buffer = nullptr;
    ri_buffer* m_build_index_buffer = nullptr;

    VkAccelerationStructureGeometryKHR m_geometry = {};

    bool m_dirty = false;
    bool m_pending_compact = false;
    bool m_is_compacted = false;
    size_t m_build_frame_index = 0;

    size_t m_uncompacted_size = 0;
    size_t m_compacted_size = 0;

};

}; // namespace ws
