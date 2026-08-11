// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_texture.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_upload_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_tile_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_param_block.h"
#include "workshop.render_interface.vulkan/vulkan_ri_staging_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_types.h"
#include "workshop.core/memory/memory_tracker.h"
#include "workshop.core/math/math.h"

namespace ws {

vulkan_ri_texture::vulkan_ri_texture(vulkan_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
{
    calculate_dropped_mips();
    m_format = ri_to_vulkan(m_create_params.format);
}

vulkan_ri_texture::vulkan_ri_texture(vulkan_render_interface& renderer, const char* debug_name, const ri_texture::create_params& params, VkImage image, VkFormat image_format, ri_resource_state common_state)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
    , m_owns_image(false)
    , m_image(image)
    , m_common_state(common_state)
{
    calculate_dropped_mips();

    // Use the caller-provided real format of the wrapped image rather than deriving it from
    // create_params.format - the swapchain surface format may not match ri_to_vulkan(create_params.format)
    // exactly (eg. falling back to a non-SRGB surface format), and vkCreateImageView requires an
    // exact format match against the wrapped VkImage unless VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT was set.
    m_format = image_format;

    m_renderer.set_debug_object_name(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(m_image), m_debug_name.c_str());

    create_views();

    // Wrapped images (eg. swapchain backbuffers) genuinely start out in VK_IMAGE_LAYOUT_UNDEFINED
    // per the Vulkan spec, same as an owned image - resolve it eagerly here for the same reason
    // as the owned-image path in create_resources(): so no render pass can race to claim the
    // one-time transition out of it. Queued through the upload manager (flushed in a single
    // batch alongside real uploads, before the next command list submission) rather than an
    // immediate per-texture submit-and-wait, since this can be called from any thread and a
    // blocking round trip per texture doesn't scale when many are created in a burst.
    m_renderer.get_upload_manager().queue_transition(m_image, get_aspect_mask(), ri_resource_state::initial, m_common_state);
    clear_undefined_layout();
}

vulkan_ri_texture::~vulkan_ri_texture()
{
    {
        std::scoped_lock lock(m_reference_mutex);

        for (vulkan_ri_param_block* ref : m_referencing_param_blocks)
        {
            ref->clear_texture_references(this);
        }
    }

    if (m_create_params.is_partially_resident)
    {
        vulkan_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();

        if (m_packed_mips_resident)
        {
            tile_manager.free_tiles(get_first_packed_mip_residency()->tile_allocation);
        }

        for (mip_residency& mip : m_mip_residency)
        {
            if (mip.is_resident && !mip.is_packed)
            {
                tile_manager.free_tiles(mip.tile_allocation);
                mip.tile_allocation = {};
                mip.is_resident = false;
            }
        }
    }

    free_views();

    if (m_owns_image)
    {
        m_renderer.defer_delete([renderer = &m_renderer, image = m_image, memory = m_memory]()
        {
            if (image != VK_NULL_HANDLE)
            {
                vkDestroyImage(renderer->get_device(), image, nullptr);
            }
            if (memory != VK_NULL_HANDLE)
            {
                vkFreeMemory(renderer->get_device(), memory, nullptr);
            }
        });
    }
}

void vulkan_ri_texture::add_param_block_reference(vulkan_ri_param_block* block)
{
    std::scoped_lock lock(m_reference_mutex);
    m_referencing_param_blocks.push_back(block);
}

void vulkan_ri_texture::remove_param_block_reference(vulkan_ri_param_block* block)
{
    std::scoped_lock lock(m_reference_mutex);
    if (auto iter = std::find(m_referencing_param_blocks.begin(), m_referencing_param_blocks.end(), block); iter != m_referencing_param_blocks.end())
    {
        m_referencing_param_blocks.erase(iter);
    }
}

bool vulkan_ri_texture::calculate_linear_data_mip_range(size_t array_index, size_t mip_index, size_t& offset, size_t& size)
{
    size_t block_size = ri_format_block_size(m_create_params.format);
    size_t face_count = m_create_params.depth;
    size_t mip_count = m_create_params.mip_levels;
    size_t dropped_mip_count = m_create_params.drop_mips;
    if (m_create_params.dimensions == ri_texture_dimension::texture_cube)
    {
        face_count = 6;
    }

    size_t undropped_width = m_create_params.width;
    size_t undropped_height = m_create_params.height;
    for (size_t i = 0; i < dropped_mip_count; i++)
    {
        undropped_width *= 2;
        undropped_height *= 2;
    }

    size_t data_offset = 0;
    for (size_t face = 0; face < face_count; face++)
    {
        size_t mip_width = undropped_width;
        size_t mip_height = undropped_height;

        for (size_t mip = 0; mip < mip_count + dropped_mip_count; mip++)
        {
            const size_t mip_size = (ri_bytes_per_texel(m_create_params.format) * mip_width * mip_height) / block_size;

            if (mip == (mip_index + dropped_mip_count) && array_index == face)
            {
                offset = data_offset;
                size = mip_size;
                return true;
            }

            data_offset += mip_size;

            mip_width = std::max(size_t{1}, mip_width / 2);
            mip_height = std::max(size_t{1}, mip_height / 2);
        }
    }

    return false;
}

VkImageAspectFlags vulkan_ri_texture::get_aspect_mask() const
{
    if (!ri_is_format_depth_target(m_create_params.format))
    {
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (m_create_params.format == ri_texture_format::D24_UNORM_S8_UINT)
    {
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    return VK_IMAGE_ASPECT_DEPTH_BIT;
}

result<void> vulkan_ri_texture::create_resources()
{
    m_memory_type = memory_type::rendering__vram__texture;
    if (m_create_params.is_render_target)
    {
        m_memory_type = ri_is_format_depth_target(m_create_params.format)
            ? memory_type::rendering__vram__render_target_depth
            : memory_type::rendering__vram__render_target_color;
    }

    memory_scope mem_scope(m_memory_type, string_hash::empty, string_hash(m_debug_name));

    db_assert_message(!m_create_params.is_partially_resident || m_create_params.dimensions == ri_texture_dimension::texture_2d, "Only 2d textures support partial residency (for now).");

    bool is_cube = (m_create_params.dimensions == ri_texture_dimension::texture_cube);
    bool is_3d = (m_create_params.dimensions == ri_texture_dimension::texture_3d);

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = ri_to_vulkan_image_type(m_create_params.dimensions);
    image_create_info.format = m_format;
    image_create_info.extent.width = static_cast<uint32_t>(m_create_params.width);
    image_create_info.extent.height = static_cast<uint32_t>(m_create_params.height);
    image_create_info.extent.depth = is_3d ? static_cast<uint32_t>(m_create_params.depth) : 1;
    image_create_info.mipLevels = static_cast<uint32_t>(m_create_params.mip_levels);
    image_create_info.arrayLayers = is_3d ? 1 : (is_cube ? 6 : static_cast<uint32_t>(m_create_params.depth));
    image_create_info.samples = static_cast<VkSampleCountFlagBits>(m_create_params.multisample_count > 0 ? m_create_params.multisample_count : 1);
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (m_create_params.is_render_target)
    {
        image_create_info.usage |= ri_is_format_depth_target(m_create_params.format)
            ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (m_create_params.allow_unordered_access)
    {
        image_create_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if (is_cube)
    {
        image_create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    if (is_3d && m_create_params.is_render_target)
    {
        image_create_info.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
    }
    if (m_create_params.is_partially_resident)
    {
        image_create_info.flags |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
    }

    VkResult vk_result = vkCreateImage(m_renderer.get_device(), &image_create_info, nullptr, &m_image);
    if (!m_renderer.check_result(vk_result, "vkCreateImage"))
    {
        return standard_errors::failed;
    }

    m_renderer.set_debug_object_name(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(m_image), m_debug_name.c_str());

    m_common_state = ri_resource_state::pixel_shader_resource;
    if (m_create_params.is_render_target)
    {
        m_common_state = ri_is_format_depth_target(m_create_params.format)
            ? ri_resource_state::depth_write
            : ri_resource_state::render_target;
    }

    if (m_create_params.is_partially_resident)
    {
        uint32_t requirement_count = 0;
        vkGetImageSparseMemoryRequirements(m_renderer.get_device(), m_image, &requirement_count, nullptr);

        std::vector<VkSparseImageMemoryRequirements> sparse_requirements(requirement_count);
        vkGetImageSparseMemoryRequirements(m_renderer.get_device(), m_image, &requirement_count, sparse_requirements.data());

        VkSparseImageMemoryRequirements color_requirements = {};
        for (const VkSparseImageMemoryRequirements& req : sparse_requirements)
        {
            if (req.formatProperties.aspectMask & get_aspect_mask())
            {
                color_requirements = req;
                break;
            }
        }

        VkExtent3D tile_extent = color_requirements.formatProperties.imageGranularity;
        uint32_t standard_mip_count = color_requirements.imageMipTailFirstLod;

        m_mip_tail_offset = color_requirements.imageMipTailOffset;
        m_mip_tail_size = color_requirements.imageMipTailSize;

        m_mip_residency.resize(m_create_params.mip_levels);

        for (size_t i = 0; i < m_create_params.mip_levels; i++)
        {
            mip_residency& residency = m_mip_residency[i];
            residency.index = i;
            residency.is_resident = false;

            if (i < standard_mip_count)
            {
                uint32_t mip_width = std::max(1u, static_cast<uint32_t>(m_create_params.width) >> i);
                uint32_t mip_height = std::max(1u, static_cast<uint32_t>(m_create_params.height) >> i);

                uint32_t tiles_wide = (mip_width + tile_extent.width - 1) / tile_extent.width;
                uint32_t tiles_high = (mip_height + tile_extent.height - 1) / tile_extent.height;

                residency.is_packed = false;
                residency.pixel_extent = { mip_width, mip_height, 1 };
                residency.tile_count = tiles_wide * tiles_high;
            }
            else
            {
                residency.is_packed = true;
                residency.tile_count = (color_requirements.imageMipTailSize + color_requirements.formatProperties.imageGranularity.width - 1) / color_requirements.formatProperties.imageGranularity.width;
                residency.tile_count = std::max(size_t{1}, residency.tile_count);
            }
        }

        begin_mip_residency_change();

        for (size_t i = 0; i < m_create_params.resident_mips; i++)
        {
            std::vector<uint8_t> mip_data;
            size_t mip_index = m_create_params.mip_levels - (i + 1);
            size_t mip_offset = 0;
            size_t mip_size = 0;

            if (!m_create_params.data.empty())
            {
                calculate_linear_data_mip_range(0, mip_index, mip_offset, mip_size);
                mip_data.assign(m_create_params.data.begin() + mip_offset, m_create_params.data.begin() + mip_offset + mip_size);
            }

            make_mip_resident(mip_index, mip_data);
        }

        end_mip_residency_change();

        m_memory_allocation_info = mem_scope.record_alloc(calculate_resident_mip_used_bytes());
    }
    else
    {
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(m_renderer.get_device(), m_image, &memory_requirements);

        result<uint32_t> memory_type_index = m_renderer.find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (!memory_type_index)
        {
            return standard_errors::failed;
        }

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = memory_type_index.get_result();

        vk_result = vkAllocateMemory(m_renderer.get_device(), &allocate_info, nullptr, &m_memory);
        if (!m_renderer.check_result(vk_result, "vkAllocateMemory"))
        {
            return standard_errors::failed;
        }

        vk_result = vkBindImageMemory(m_renderer.get_device(), m_image, m_memory, 0);
        if (!m_renderer.check_result(vk_result, "vkBindImageMemory"))
        {
            return standard_errors::failed;
        }

        m_memory_allocation_info = mem_scope.record_alloc(memory_requirements.size);

        if (!m_create_params.data.empty())
        {
            m_renderer.get_upload_manager().upload(*this, m_create_params.data);
        }
    }

    m_pitch = math::round_up_multiple(ri_bytes_per_texel(m_create_params.format) * m_create_params.width, size_t{256});

    create_views();

    // Render targets are transitioned out of VK_IMAGE_LAYOUT_UNDEFINED here, before the texture
    // is ever handed to a pass, rather than lazily on first use inside
    // vulkan_ri_command_list::barrier(). Render passes are recorded in parallel
    // (render_pass::generate() runs on the task scheduler), and the lazy path resolved which
    // pass "won" the one-time undefined transition based on whichever thread reached barrier()
    // first - not on which pass actually executes first this frame - so a pass could legitimately
    // be skipped its needed transition. Queued through the upload manager (flushed in a single
    // batch before the next command list submission) rather than an immediate per-texture
    // submit-and-wait - this constructor can run on any worker thread (eg. asset loading), and
    // many textures are often created in a burst, so a blocking round trip per texture doesn't
    // scale and needlessly serializes otherwise-parallel work.
    if (m_create_params.is_render_target && !m_create_params.is_partially_resident)
    {
        m_renderer.get_upload_manager().queue_transition(m_image, get_aspect_mask(), ri_resource_state::initial, m_common_state);
        clear_undefined_layout();
    }

    return true;
}

const vulkan_ri_texture::mip_residency& vulkan_ri_texture::get_mip_residency(size_t index)
{
    db_assert(m_create_params.is_partially_resident);
    return m_mip_residency[index];
}

size_t vulkan_ri_texture::get_max_resident_mip()
{
    if (!m_create_params.is_partially_resident)
    {
        return 0;
    }

    if (m_mip_residency.empty())
    {
        return 0;
    }

    for (int32_t i = (int32_t)m_mip_residency.size() - 1; i >= 0; i--)
    {
        if (!m_mip_residency[i].is_resident)
        {
            return i + 1;
        }
    }

    return 0;
}

vulkan_ri_texture::mip_residency* vulkan_ri_texture::get_first_packed_mip_residency()
{
    for (size_t i = 0; i < m_mip_residency.size(); i++)
    {
        if (m_mip_residency[i].is_packed)
        {
            return &m_mip_residency[i];
        }
    }
    return nullptr;
}

size_t vulkan_ri_texture::calculate_resident_mip_used_bytes()
{
    size_t total = 0;
    bool added_packed_tiles = false;

    for (mip_residency& mip : m_mip_residency)
    {
        if (mip.is_resident)
        {
            if (!mip.is_packed || !added_packed_tiles)
            {
                total += mip.tile_count * 65536;
                if (mip.is_packed)
                {
                    added_packed_tiles = true;
                }
            }
        }
    }

    return total;
}

void vulkan_ri_texture::update_packed_mip_chain_residency()
{
    vulkan_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();

    bool should_packed_mips_be_resident = false;
    size_t packed_tile_count = 0;
    size_t first_packed_mip_index = std::numeric_limits<size_t>::max();

    for (mip_residency& mip : m_mip_residency)
    {
        if (mip.is_resident && mip.is_packed)
        {
            should_packed_mips_be_resident = true;
            packed_tile_count = mip.tile_count;
        }

        if (mip.is_packed && first_packed_mip_index == std::numeric_limits<size_t>::max())
        {
            first_packed_mip_index = mip.index;
        }
    }

    if (should_packed_mips_be_resident == m_packed_mips_resident)
    {
        return;
    }

    mip_residency* first_packed = get_first_packed_mip_residency();

    if (should_packed_mips_be_resident)
    {
        first_packed->tile_allocation = tile_manager.allocate_tiles(packed_tile_count);
        tile_manager.queue_map(*this, first_packed->tile_allocation, first_packed_mip_index);
    }
    else
    {
        tile_manager.queue_unmap(*this, first_packed_mip_index);
        tile_manager.free_tiles(first_packed->tile_allocation);
        first_packed->tile_allocation = {};
    }

    m_packed_mips_resident = should_packed_mips_be_resident;
}

size_t vulkan_ri_texture::get_memory_usage_with_residency(size_t mip_count)
{
    size_t total_tiles = 0;
    bool added_packed_tiles = false;

    if (mip_count > m_mip_residency.size())
    {
        mip_count = m_mip_residency.size();
    }

    for (size_t i = m_mip_residency.size() - mip_count; i < m_mip_residency.size(); i++)
    {
        mip_residency& mip = m_mip_residency[i];
        if (!mip.is_packed || !added_packed_tiles)
        {
            total_tiles += mip.tile_count;

            if (mip.is_packed)
            {
                added_packed_tiles = true;
            }
        }
    }

    return total_tiles * 65536;
}

void vulkan_ri_texture::begin_mip_residency_change()
{
    m_in_mip_residency_change = true;
}

void vulkan_ri_texture::end_mip_residency_change()
{
    m_in_mip_residency_change = false;

    if (m_views_pending_recreate)
    {
        m_views_pending_recreate = false;

        if (m_main_srv.is_valid)
        {
            recreate_views();
        }
    }
}

void vulkan_ri_texture::make_mip_resident(size_t mip_index, const std::span<uint8_t>& linear_data)
{
    db_assert(m_in_mip_residency_change);
    db_assert(m_create_params.is_partially_resident);

    mip_residency* residency = &m_mip_residency[mip_index];

    vulkan_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();
    vulkan_ri_upload_manager& upload_manager = m_renderer.get_upload_manager();

    if (!residency->is_resident)
    {
        residency->is_resident = true;

        if (residency->is_packed)
        {
            update_packed_mip_chain_residency();
        }
        else
        {
            residency->tile_allocation = tile_manager.allocate_tiles(residency->tile_count);
            tile_manager.queue_map(*this, residency->tile_allocation, residency->index);
        }
    }

    if (!linear_data.empty())
    {
        upload_manager.upload(*this, 0, mip_index, linear_data);
    }

    m_views_pending_recreate = true;

    memory_scope mem_scope(m_memory_type, string_hash::empty, string_hash(m_debug_name));
    m_memory_allocation_info = mem_scope.record_alloc(calculate_resident_mip_used_bytes());
}

void vulkan_ri_texture::make_mip_resident(size_t mip_index, ri_staging_buffer& data_buffer)
{
    db_assert(m_in_mip_residency_change);
    db_assert(m_create_params.is_partially_resident);

    mip_residency* residency = &m_mip_residency[mip_index];

    vulkan_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();
    vulkan_ri_upload_manager& upload_manager = m_renderer.get_upload_manager();

    if (!residency->is_resident)
    {
        residency->is_resident = true;

        if (residency->is_packed)
        {
            update_packed_mip_chain_residency();
        }
        else
        {
            residency->tile_allocation = tile_manager.allocate_tiles(residency->tile_count);
            tile_manager.queue_map(*this, residency->tile_allocation, residency->index);
        }
    }

    upload_manager.upload(*this, 0, mip_index, static_cast<vulkan_ri_staging_buffer&>(data_buffer));

    m_views_pending_recreate = true;

    memory_scope mem_scope(m_memory_type, string_hash::empty, string_hash(m_debug_name));
    m_memory_allocation_info = mem_scope.record_alloc(calculate_resident_mip_used_bytes());
}

void vulkan_ri_texture::make_mip_non_resident(size_t mip_index)
{
    db_assert(m_in_mip_residency_change);
    db_assert(m_create_params.is_partially_resident);

    mip_residency& residency = m_mip_residency[mip_index];

    if (!residency.is_resident)
    {
        return;
    }

    residency.is_resident = false;

    if (residency.is_packed)
    {
        update_packed_mip_chain_residency();
    }
    else
    {
        vulkan_ri_tile_manager& tile_manager = m_renderer.get_tile_manager();
        tile_manager.queue_unmap(*this, mip_index);
        tile_manager.free_tiles(residency.tile_allocation);
        residency.tile_allocation = {};
    }

    m_views_pending_recreate = true;

    memory_scope mem_scope(m_memory_type, string_hash::empty, string_hash(m_debug_name));
    m_memory_allocation_info = mem_scope.record_alloc(calculate_resident_mip_used_bytes());
}

void vulkan_ri_texture::calculate_dropped_mips()
{
    if (!m_create_params.data.empty() && m_create_params.drop_mips > 0)
    {
        size_t to_drop = m_create_params.drop_mips;
        m_create_params.drop_mips = 0;

        while (m_create_params.width >= 4 &&
            m_create_params.height >= 4 &&
            m_create_params.mip_levels >= 2 &&
            to_drop > 0)
        {
            m_create_params.width /= 2;
            m_create_params.height /= 2;
            m_create_params.drop_mips++;
            m_create_params.mip_levels--;
            to_drop--;
        }
    }
    else
    {
        m_create_params.drop_mips = 0;
    }

    m_create_params.resident_mips = std::min(m_create_params.resident_mips, m_create_params.mip_levels);
}

void vulkan_ri_texture::recreate_views()
{
    free_views();
    create_views();
}

void vulkan_ri_texture::create_views()
{
    size_t mip_levels = m_create_params.mip_levels;
    bool is_cube = (m_create_params.dimensions == ri_texture_dimension::texture_cube);
    bool is_3d = (m_create_params.dimensions == ri_texture_dimension::texture_3d);
    size_t slice_count = (is_cube || is_3d) ? m_create_params.depth : 1;

    m_srv_table = ri_descriptor_table::texture_2d;
    switch (m_create_params.dimensions)
    {
    case ri_texture_dimension::texture_1d:
        m_srv_table = ri_descriptor_table::texture_1d;
        break;
    case ri_texture_dimension::texture_3d:
        m_srv_table = ri_descriptor_table::texture_3d;
        break;
    case ri_texture_dimension::texture_cube:
        m_srv_table = ri_descriptor_table::texture_cube;
        break;
    default:
        break;
    }

    VkImageAspectFlags full_aspect = get_aspect_mask();
    VkImageAspectFlags sample_aspect = ri_is_format_depth_target(m_create_params.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    // Render target / depth stencil views - raw VkImageViews consumed directly by dynamic
    // rendering, not registered in a bindless descriptor table.
    if (m_create_params.is_render_target)
    {
        if (ri_is_format_depth_target(m_create_params.format))
        {
            m_dsv_views.resize(std::max(size_t{1}, slice_count));
            for (size_t slice = 0; slice < m_dsv_views.size(); slice++)
            {
                VkImageViewCreateInfo view_create_info = {};
                view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                view_create_info.image = m_image;
                view_create_info.viewType = (is_3d || is_cube) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                view_create_info.format = m_format;
                view_create_info.subresourceRange.aspectMask = full_aspect;
                view_create_info.subresourceRange.baseMipLevel = 0;
                view_create_info.subresourceRange.levelCount = 1;
                view_create_info.subresourceRange.baseArrayLayer = static_cast<uint32_t>(slice);
                view_create_info.subresourceRange.layerCount = 1;

                vkCreateImageView(m_renderer.get_device(), &view_create_info, nullptr, &m_dsv_views[slice]);
            }
        }
        else
        {
            m_rtv_views.resize(mip_levels);
            for (size_t mip = 0; mip < mip_levels; mip++)
            {
                m_rtv_views[mip].resize(std::max(size_t{1}, slice_count));
                for (size_t slice = 0; slice < m_rtv_views[mip].size(); slice++)
                {
                    VkImageViewCreateInfo view_create_info = {};
                    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    view_create_info.image = m_image;
                    view_create_info.viewType = (is_3d || is_cube) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                    view_create_info.format = m_format;
                    view_create_info.subresourceRange.aspectMask = full_aspect;
                    view_create_info.subresourceRange.baseMipLevel = static_cast<uint32_t>(mip);
                    view_create_info.subresourceRange.levelCount = 1;
                    view_create_info.subresourceRange.baseArrayLayer = static_cast<uint32_t>(slice);
                    view_create_info.subresourceRange.layerCount = 1;

                    vkCreateImageView(m_renderer.get_device(), &view_create_info, nullptr, &m_rtv_views[mip][slice]);
                }
            }
        }
    }

    // Unordered access (storage image) views - one bindless slot per mip/slice.
    if (m_create_params.allow_unordered_access)
    {
        m_uav_views.resize(mip_levels);
        m_uavs.resize(mip_levels);

        for (size_t mip = 0; mip < mip_levels; mip++)
        {
            m_uav_views[mip].resize(std::max(size_t{1}, slice_count));
            m_uavs[mip].resize(std::max(size_t{1}, slice_count));

            for (size_t slice = 0; slice < m_uav_views[mip].size(); slice++)
            {
                VkImageViewCreateInfo view_create_info = {};
                view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                view_create_info.image = m_image;
                view_create_info.viewType = (is_3d || is_cube) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                view_create_info.format = m_format;
                view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                view_create_info.subresourceRange.baseMipLevel = static_cast<uint32_t>(mip);
                view_create_info.subresourceRange.levelCount = 1;
                view_create_info.subresourceRange.baseArrayLayer = static_cast<uint32_t>(slice);
                view_create_info.subresourceRange.layerCount = 1;

                vkCreateImageView(m_renderer.get_device(), &view_create_info, nullptr, &m_uav_views[mip][slice]);

                m_uavs[mip][slice] = m_renderer.get_descriptor_table(ri_descriptor_table::rwtexture_2d).allocate();
                m_renderer.get_descriptor_table(ri_descriptor_table::rwtexture_2d).write_storage_image(m_uavs[mip][slice], m_uav_views[mip][slice]);
            }
        }
    }

    // Individual mip/slice SRVs - one bindless slot per mip/slice.
    if (m_create_params.allow_individual_image_access)
    {
        m_srv_views.resize(mip_levels);
        m_srvs.resize(mip_levels);

        for (size_t mip = 0; mip < mip_levels; mip++)
        {
            m_srv_views[mip].resize(std::max(size_t{1}, slice_count));
            m_srvs[mip].resize(std::max(size_t{1}, slice_count));

            for (size_t slice = 0; slice < m_srv_views[mip].size(); slice++)
            {
                VkImageViewCreateInfo view_create_info = {};
                view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                view_create_info.image = m_image;
                view_create_info.viewType = (is_3d || is_cube) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                view_create_info.format = m_format;
                view_create_info.subresourceRange.aspectMask = sample_aspect;
                view_create_info.subresourceRange.baseMipLevel = static_cast<uint32_t>(mip);
                view_create_info.subresourceRange.levelCount = 1;
                view_create_info.subresourceRange.baseArrayLayer = static_cast<uint32_t>(slice);
                view_create_info.subresourceRange.layerCount = 1;

                vkCreateImageView(m_renderer.get_device(), &view_create_info, nullptr, &m_srv_views[mip][slice]);

                m_srvs[mip][slice] = m_renderer.get_descriptor_table(ri_descriptor_table::texture_2d).allocate();
                m_renderer.get_descriptor_table(ri_descriptor_table::texture_2d).write_sampled_image(m_srvs[mip][slice], m_srv_views[mip][slice]);
            }
        }
    }

    // Main SRV - covers all currently-resident mips, this is what bindless texture sampling
    // in shaders reads through.
    uint32_t most_detailed_mip = static_cast<uint32_t>(get_max_resident_mip());

    VkImageViewCreateInfo main_view_create_info = {};
    main_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    main_view_create_info.image = m_image;
    main_view_create_info.viewType = ri_to_vulkan_view_type(m_create_params.dimensions);
    main_view_create_info.format = m_format;
    main_view_create_info.subresourceRange.aspectMask = sample_aspect;
    main_view_create_info.subresourceRange.baseMipLevel = most_detailed_mip;
    main_view_create_info.subresourceRange.levelCount = static_cast<uint32_t>(mip_levels) - most_detailed_mip;
    main_view_create_info.subresourceRange.baseArrayLayer = 0;
    main_view_create_info.subresourceRange.layerCount = is_cube ? 6 : (is_3d ? 1 : static_cast<uint32_t>(m_create_params.depth));

    vkCreateImageView(m_renderer.get_device(), &main_view_create_info, nullptr, &m_main_srv_view);

    m_main_srv = m_renderer.get_descriptor_table(m_srv_table).allocate();
    m_renderer.get_descriptor_table(m_srv_table).write_sampled_image(m_main_srv, m_main_srv_view);

    {
        std::scoped_lock lock(m_reference_mutex);

        for (vulkan_ri_param_block* ref : m_referencing_param_blocks)
        {
            ref->referenced_texture_modified(this);
        }
    }
}

void vulkan_ri_texture::free_views()
{
    m_renderer.defer_delete([renderer = &m_renderer, srv_table = m_srv_table, main_srv = m_main_srv, main_srv_view = m_main_srv_view,
        srv_views = m_srv_views, srvs = m_srvs, rtv_views = m_rtv_views, uav_views = m_uav_views, uavs = m_uavs, dsv_views = m_dsv_views]()
    {
        if (main_srv.is_valid)
        {
            renderer->get_descriptor_table(srv_table).free(main_srv);
        }
        if (main_srv_view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(renderer->get_device(), main_srv_view, nullptr);
        }

        for (size_t mip = 0; mip < srv_views.size(); mip++)
        {
            for (size_t slice = 0; slice < srv_views[mip].size(); slice++)
            {
                if (srvs[mip][slice].is_valid)
                {
                    renderer->get_descriptor_table(ri_descriptor_table::texture_2d).free(srvs[mip][slice]);
                }
                if (srv_views[mip][slice] != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(renderer->get_device(), srv_views[mip][slice], nullptr);
                }
            }
        }

        for (auto& mip_rtvs : rtv_views)
        {
            for (VkImageView view : mip_rtvs)
            {
                if (view != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(renderer->get_device(), view, nullptr);
                }
            }
        }

        for (size_t mip = 0; mip < uav_views.size(); mip++)
        {
            for (size_t slice = 0; slice < uav_views[mip].size(); slice++)
            {
                if (uavs[mip][slice].is_valid)
                {
                    renderer->get_descriptor_table(ri_descriptor_table::rwtexture_2d).free(uavs[mip][slice]);
                }
                if (uav_views[mip][slice] != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(renderer->get_device(), uav_views[mip][slice], nullptr);
                }
            }
        }

        for (VkImageView view : dsv_views)
        {
            if (view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(renderer->get_device(), view, nullptr);
            }
        }
    });

    m_main_srv = {};
    m_main_srv_view = VK_NULL_HANDLE;
    m_srv_views.clear();
    m_srvs.clear();
    m_rtv_views.clear();
    m_uav_views.clear();
    m_uavs.clear();
    m_dsv_views.clear();
}

vulkan_ri_descriptor_table::allocation vulkan_ri_texture::get_main_srv() const
{
    return m_main_srv;
}

vulkan_ri_descriptor_table::allocation vulkan_ri_texture::get_srv(size_t slice, size_t mip) const
{
    db_assert(m_create_params.allow_individual_image_access);
    return m_srvs[mip][slice];
}

vulkan_ri_descriptor_table::allocation vulkan_ri_texture::get_uav(size_t slice, size_t mip) const
{
    db_assert(m_create_params.allow_unordered_access);
    return m_uavs[mip][slice];
}

VkImageView vulkan_ri_texture::get_rtv_view(size_t slice, size_t mip) const
{
    return m_rtv_views[mip][slice];
}

VkImageView vulkan_ri_texture::get_dsv_view(size_t slice) const
{
    return m_dsv_views[slice];
}

VkImage vulkan_ri_texture::get_image()
{
    return m_image;
}

VkDeviceSize vulkan_ri_texture::get_mip_tail_offset() const
{
    return m_mip_tail_offset;
}

VkDeviceSize vulkan_ri_texture::get_mip_tail_size() const
{
    return m_mip_tail_size;
}

size_t vulkan_ri_texture::get_pitch()
{
    return m_pitch;
}

size_t vulkan_ri_texture::get_width()
{
    return m_create_params.width;
}

size_t vulkan_ri_texture::get_height()
{
    return m_create_params.height;
}

size_t vulkan_ri_texture::get_depth()
{
    return m_create_params.depth;
}

size_t vulkan_ri_texture::get_mip_levels()
{
    return m_create_params.mip_levels;
}

size_t vulkan_ri_texture::get_dropped_mips()
{
    return m_create_params.drop_mips;
}

ri_texture_dimension vulkan_ri_texture::get_dimensions() const
{
    return m_create_params.dimensions;
}

ri_texture_format vulkan_ri_texture::get_format()
{
    return m_create_params.format;
}

const char* vulkan_ri_texture::get_debug_name()
{
    return m_debug_name.c_str();
}

size_t vulkan_ri_texture::get_multisample_count()
{
    return m_create_params.multisample_count;
}

color vulkan_ri_texture::get_optimal_clear_color()
{
    return m_create_params.optimal_clear_color;
}

float vulkan_ri_texture::get_optimal_clear_depth()
{
    return m_create_params.optimal_clear_depth;
}

uint8_t vulkan_ri_texture::get_optimal_clear_stencil()
{
    return m_create_params.optimal_clear_stencil;
}

bool vulkan_ri_texture::is_render_target()
{
    return m_create_params.is_render_target;
}

bool vulkan_ri_texture::is_depth_stencil()
{
    return ri_is_format_depth_target(m_create_params.format);
}

bool vulkan_ri_texture::is_partially_resident() const
{
    return m_create_params.is_partially_resident;
}

size_t vulkan_ri_texture::get_resident_mips()
{
    for (size_t i = 0; i < m_mip_residency.size(); i++)
    {
        size_t mip_index = m_mip_residency.size() - (i + 1);
        if (!m_mip_residency[mip_index].is_resident)
        {
            return i;
        }
    }
    return m_mip_residency.size();
}

bool vulkan_ri_texture::is_mip_resident(size_t mip_index)
{
    return m_mip_residency[mip_index].is_resident;
}

void vulkan_ri_texture::get_mip_source_data_range(size_t mip_index, size_t& offset, size_t& size)
{
    calculate_linear_data_mip_range(0, mip_index, offset, size);
}

ri_resource_state vulkan_ri_texture::get_initial_state()
{
    return m_common_state;
}

bool vulkan_ri_texture::has_undefined_layout() const
{
    return m_undefined_layout;
}

void vulkan_ri_texture::clear_undefined_layout()
{
    m_undefined_layout = false;
}

void vulkan_ri_texture::swap(ri_texture* other)
{
    vulkan_ri_texture* vulkan_other = static_cast<vulkan_ri_texture*>(other);

    std::swap(m_debug_name, vulkan_other->m_debug_name);
    std::swap(m_create_params, vulkan_other->m_create_params);

    std::swap(m_srv_table, vulkan_other->m_srv_table);
    std::swap(m_common_state, vulkan_other->m_common_state);
    std::swap(m_undefined_layout, vulkan_other->m_undefined_layout);
    std::swap(m_format, vulkan_other->m_format);

    std::swap(m_owns_image, vulkan_other->m_owns_image);
    std::swap(m_image, vulkan_other->m_image);
    std::swap(m_memory, vulkan_other->m_memory);

    recreate_views();
}

}; // namespace ws
