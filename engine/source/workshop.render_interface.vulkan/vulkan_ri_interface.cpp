// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_swapchain.h"
#include "workshop.render_interface.vulkan/vulkan_ri_fence.h"
#include "workshop.render_interface.vulkan/vulkan_ri_shader_compiler.h"
#include "workshop.render_interface.vulkan/vulkan_ri_texture_compiler.h"
#include "workshop.render_interface.vulkan/vulkan_ri_pipeline.h"
#include "workshop.render_interface.vulkan/vulkan_ri_param_block_archetype.h"
#include "workshop.render_interface.vulkan/vulkan_ri_texture.h"
#include "workshop.render_interface.vulkan/vulkan_ri_sampler.h"
#include "workshop.render_interface.vulkan/vulkan_ri_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_ri_layout_factory.h"
#include "workshop.render_interface.vulkan/vulkan_ri_query.h"
#include "workshop.render_interface.vulkan/vulkan_ri_raytracing_blas.h"
#include "workshop.render_interface.vulkan/vulkan_ri_raytracing_tlas.h"
#include "workshop.render_interface.vulkan/vulkan_ri_staging_buffer.h"
#include "workshop.render_interface.vulkan/vulkan_ri_tile_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_query_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_upload_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_param_block.h"
#include "workshop.core/app/app.h"
#include "workshop.core/filesystem/file.h"

namespace ws {
namespace
{

VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        db_error(render_interface, "%s", callback_data->pMessage);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        db_warning(render_interface, "%s", callback_data->pMessage);
    }
    else
    {
        db_verbose(render_interface, "%s", callback_data->pMessage);
    }

    return VK_FALSE;
}

}; // anon namespace

vulkan_render_interface::vulkan_render_interface(size_t ray_type_count, size_t ray_domain_count)
    : m_ray_type_count(ray_type_count)
    , m_ray_domain_count(ray_domain_count)
{
}

vulkan_render_interface::~vulkan_render_interface() = default;

void vulkan_render_interface::register_init(init_list& list)
{
    list.add_step(
        "Create Vulkan Device",
        [this]() -> result<void> { return create_device(); },
        [this]() -> result<void> { return destroy_device(); }
    );
    list.add_step(
        "Create Vulkan Command Queues",
        [this]() -> result<void> { return create_command_queues(); },
        [this]() -> result<void> { return destroy_command_queues(); }
    );
    list.add_step(
        "Create Vulkan Heaps",
        [this]() -> result<void> { return create_heaps(); },
        [this]() -> result<void> { return destroy_heaps(); }
    );
    list.add_step(
        "Create Vulkan Misc",
        [this]() -> result<void> { return create_misc(); },
        [this]() -> result<void> { return destroy_misc(); }
    );
}

void vulkan_render_interface::begin_frame()
{
    m_frame_index++;

    process_pending_deletes();

    m_graphics_queue->begin_frame();
    m_copy_queue->begin_frame();
    m_query_manager->begin_frame();
}

void vulkan_render_interface::end_frame()
{
    m_graphics_queue->end_frame();
    m_copy_queue->end_frame();
}

void vulkan_render_interface::flush_uploads()
{
    profile_marker(profile_colors::render, "flush uploads");

    // Don't allow re-entry.
    if (m_flush_upload_reentry)
    {
        return;
    }

    m_flush_upload_reentry = true;

    // Upload the state of any dirty param blocks.
    {
        std::scoped_lock lock(m_dirty_param_block_mutex);
        for (vulkan_ri_param_block* block : m_dirty_param_blocks)
        {
            block->upload_state();
        }

        m_dirty_param_blocks.clear();
    }

    // Always flush the tile manager before the upload manager as it will likely
    // be trying to update mappings to upload to.
    m_tile_manager->new_frame(m_frame_index);

    m_upload_manager->new_frame(m_frame_index);

    m_flush_upload_reentry = false;
}

std::unique_ptr<ri_swapchain> vulkan_render_interface::create_swapchain(window& for_window, const char* debug_name)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_swapchain>(for_window, debug_name);
}

std::unique_ptr<ri_fence> vulkan_render_interface::create_fence(const char* debug_name)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_fence>();
}

std::unique_ptr<ri_shader_compiler> vulkan_render_interface::create_shader_compiler()
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_shader_compiler>();
}

std::unique_ptr<ri_texture_compiler> vulkan_render_interface::create_texture_compiler()
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_texture_compiler>();
}

std::unique_ptr<ri_pipeline> vulkan_render_interface::create_pipeline(const ri_pipeline::create_params& params, const char* debug_name)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_pipeline>(params, debug_name);
}

std::unique_ptr<ri_param_block_archetype> vulkan_render_interface::create_param_block_archetype(const ri_param_block_archetype::create_params& params, const char* debug_name)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_param_block_archetype>(params, debug_name);
}

std::unique_ptr<ri_texture> vulkan_render_interface::create_texture(const ri_texture::create_params& params, const char* debug_name)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_texture>(params, debug_name);
}

std::unique_ptr<ri_sampler> vulkan_render_interface::create_sampler(const ri_sampler::create_params& params, const char* debug_name)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_sampler>(params, debug_name);
}

std::unique_ptr<ri_buffer> vulkan_render_interface::create_buffer(const ri_buffer::create_params& params, const char* debug_name)
{
    std::unique_ptr<vulkan_ri_buffer> instance = std::make_unique<vulkan_ri_buffer>(*this, debug_name, params);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_layout_factory> vulkan_render_interface::create_layout_factory(ri_data_layout layout, ri_layout_usage usage)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_layout_factory>(*this, layout, usage);
}

std::unique_ptr<ri_query> vulkan_render_interface::create_query(const ri_query::create_params& params, const char* debug_name)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_query>(params, debug_name);
}

std::unique_ptr<ri_raytracing_blas> vulkan_render_interface::create_raytracing_blas(const char* debug_name)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_raytracing_blas>();
}

std::unique_ptr<ri_raytracing_tlas> vulkan_render_interface::create_raytracing_tlas(const char* debug_name)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_raytracing_tlas>(*this, debug_name);
}

std::unique_ptr<ri_staging_buffer> vulkan_render_interface::create_staging_buffer(const ri_staging_buffer::create_params& params, std::span<uint8_t> linear_data)
{
    // vulkan-todo
    return std::make_unique<vulkan_ri_staging_buffer>(params, linear_data);
}

ri_command_queue& vulkan_render_interface::get_graphics_queue()
{
    return *m_graphics_queue;
}

ri_command_queue& vulkan_render_interface::get_copy_queue()
{
    return *m_copy_queue;
}

size_t vulkan_render_interface::get_pipeline_depth()
{
    return k_pipeline_depth;
}

void vulkan_render_interface::queue_dirty_param_block(vulkan_ri_param_block* block)
{
    std::scoped_lock lock(m_dirty_param_block_mutex);
    m_dirty_param_blocks.insert(block);
}

void vulkan_render_interface::dequeue_dirty_param_block(vulkan_ri_param_block* block)
{
    std::scoped_lock lock(m_dirty_param_block_mutex);
    m_dirty_param_blocks.erase(block);
}

std::recursive_mutex& vulkan_render_interface::get_dirty_param_block_mutex()
{
    return m_dirty_param_block_mutex;
}

void vulkan_render_interface::drain_deferred()
{
    std::scoped_lock lock(m_pending_deletion_mutex);

    for (size_t i = 0; i < k_max_pipeline_depth; i++)
    {
        auto& queue = m_pending_deletions[i];
        for (deferred_delete_function_t& functor : queue)
        {
            functor();
        }
        queue.clear();
    }
}

void vulkan_render_interface::process_pending_deletes()
{
    std::scoped_lock lock(m_pending_deletion_mutex);

    profile_marker(profile_colors::render, "process pending deletes");

    size_t queue_index = (m_frame_index % k_max_pipeline_depth);
    auto& queue = m_pending_deletions[queue_index];
    for (deferred_delete_function_t& functor : queue)
    {
        functor();
    }
    queue.clear();
}

void vulkan_render_interface::defer_delete(deferred_delete_function_t&& func)
{
    std::scoped_lock lock(m_pending_deletion_mutex);

    size_t queue_index = (m_frame_index % k_max_pipeline_depth);
    m_pending_deletions[queue_index].push_back(std::move(func));
}

void vulkan_render_interface::get_vram_usage(size_t& out_local, size_t& out_non_local)
{
    VkPhysicalDeviceMemoryProperties2 memory_properties;
    vkGetPhysicalDeviceMemoryProperties2(m_physical_device, &memory_properties);

    out_local = 0;
    out_non_local = 0;

    for (uint32_t i = 0; i < memory_properties.memoryProperties.memoryHeapCount; i++)
    {
        if (memory_properties.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            out_local += memory_properties.memoryProperties.memoryHeaps[i].size;
        }
        else
        {
            out_non_local += memory_properties.memoryProperties.memoryHeaps[i].size;
        }
    }
}

void vulkan_render_interface::get_vram_total(size_t& out_local_total, size_t& out_non_local_total)
{
    out_local_total = m_vram_total_local;
    out_non_local_total = m_vram_total_non_local;
}

size_t vulkan_render_interface::get_cube_map_face_index(ri_cube_map_face face)
{
    return static_cast<size_t>(face);
}

bool vulkan_render_interface::check_feature(ri_feature feature)
{
    return m_feature_support[(int)feature];
}

bool vulkan_render_interface::check_result(VkResult result, const char* context)
{
    if (result != VK_SUCCESS)
    {
        db_error(render_interface, "%s failed with error 0x%08x.", context, result);
        return false;
    }

    return true;
}

void vulkan_render_interface::assert_result(VkResult result, const char* context)
{
    if (result != VK_SUCCESS)
    {
        db_fatal(render_interface, "%s failed with error 0x%08x.", context, result);
    }
}

VkDevice vulkan_render_interface::get_device()
{
    return m_device;
}

VkInstance vulkan_render_interface::get_instance()
{
    return m_instance;
}

result<uint32_t> vulkan_render_interface::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    // vulkan-todo
    return standard_errors::failed;
}

vulkan_ri_descriptor_table& vulkan_render_interface::get_descriptor_table(ri_descriptor_table table)
{
    return *m_descriptor_tables[static_cast<int>(table)];
}

vulkan_ri_small_buffer_allocator& vulkan_render_interface::get_small_buffer_allocator()
{
    return *m_small_buffer_allocator;
}

vulkan_ri_upload_manager& vulkan_render_interface::get_upload_manager()
{
    return *m_upload_manager;
}

bool vulkan_render_interface::check_extension_support(const char* name)
{
    for (const VkExtensionProperties& ext : m_available_extensions)
    {
        if (strcmp(ext.extensionName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

bool vulkan_render_interface::check_physical_device_extension_support(const char* name)
{
    for (const VkExtensionProperties& ext : m_available_physical_device_extensions)
    {
        if (strcmp(ext.extensionName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

bool vulkan_render_interface::check_layer_support(const char* name)
{
    for (const VkLayerProperties& ext : m_available_layers)
    {
        if (strcmp(ext.layerName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

result<void> vulkan_render_interface::get_required_extensions()
{ 
    // Get extensions SDL requires to setup a surface.
    unsigned int sdl_extension_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(nullptr, &sdl_extension_count, nullptr))
    {
        db_error(render_interface, "SDL_Vulkan_GetInstanceExtensions failed with error: %s", SDL_GetError());
        return standard_errors::failed;
    }

    size_t offset = m_required_extensions.size();
    m_required_extensions.resize(m_required_extensions.size() + sdl_extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(nullptr, &sdl_extension_count, m_required_extensions.data() + offset))
    {
        db_error(render_interface, "SDL_Vulkan_GetInstanceExtensions failed with error: %s", SDL_GetError());
        return standard_errors::failed;
    }

    // Add debug extensions if we require them.
    if (m_debug_device)
    {
        m_required_extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
        m_required_layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    if (check_extension_support(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
        m_required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    m_required_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    // Device selected needs various extensions as well.
    m_required_physical_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    m_required_physical_device_extensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
    m_required_physical_device_extensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

    // Check extensions are supported.
    for (const char* extension : m_required_extensions)
    {
        if (!check_extension_support(extension))
        {
            db_error(render_interface, "No support available for required vulkan extension '%s'.", extension);
            return false;
        }
    }

    return true;
}

result<void> vulkan_render_interface::get_required_layers()
{
    // Add debug layers if we require them.
    if (m_debug_device)
    {
        m_required_layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    // Check layers are supported.
    for (const char* layer : m_required_layers)
    {
        if (!check_layer_support(layer))
        {
            db_error(render_interface, "No support available for required vulkan layer '%s'.", layer);
            return false;
        }
    }

    return true;
}

result<void> vulkan_render_interface::get_extensions()
{
    uint32_t available_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);

    m_available_extensions.resize(available_extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, m_available_extensions.data());

    db_log(render_interface, "Instance Extension Supported:");
    for (const VkExtensionProperties& ext : m_available_extensions)
    {
        db_log(render_interface, "\t%s", ext.extensionName);
    }

    if (result<void> ret = get_required_extensions(); !ret)
    {
        return ret;
    }

    return true;
}

result<void> vulkan_render_interface::get_layers()
{
    uint32_t available_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);

    m_available_layers.resize(available_layer_count);
    vkEnumerateInstanceLayerProperties(&available_layer_count, m_available_layers.data());

    db_log(render_interface, "Instance Layers Supported:");
    for (const VkLayerProperties& ext : m_available_layers)
    {
        db_log(render_interface, "\t%s", ext.layerName);
    }

    if (result<void> ret = get_required_layers(); !ret)
    {
        return ret;
    }

    return true;
}

result<void> vulkan_render_interface::create_device()
{
    if (ws::is_option_set("vulkan_debug"))
    {
        m_debug_device = true;
    }

    if (result<void> ret = get_extensions(); !ret)
    {
        return ret;
    }
    if (result<void> ret = get_layers(); !ret)
    {
        return ret;
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app::instance().get_name().c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "workshop";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(m_required_extensions.size());
    instance_create_info.ppEnabledExtensionNames = m_required_extensions.data();
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(m_required_layers.size());
    instance_create_info.ppEnabledLayerNames = m_required_layers.data();

    // Hook any vulkan logs during createInstance/destroyInstance
    VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = {};
    messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messenger_create_info.pfnUserCallback = debug_messenger_callback;
    if (check_extension_support(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
        instance_create_info.pNext = &messenger_create_info;
    }

    // If debugging then enable validation features.
    if (m_debug_device)
    {
        std::array<VkValidationFeatureEnableEXT, 1> enabled_validation_features = {
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        };

        VkValidationFeaturesEXT validation_features = {};
        validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        validation_features.enabledValidationFeatureCount = static_cast<uint32_t>(enabled_validation_features.size());
        validation_features.pEnabledValidationFeatures = enabled_validation_features.data();

        validation_features.pNext = instance_create_info.pNext;
        instance_create_info.pNext = &validation_features;
    }

    // Create the device!
    VkResult vk_result = vkCreateInstance(&instance_create_info, nullptr, &m_instance);
    if (!check_result(vk_result, "vkCreateInstance"))
    {
        return standard_errors::failed;
    }

    // Resolve all extension functions.
    if (result<void> ret = resolve_instance_functions(); !ret)
    {
        return ret;
    }

    // Hook any messages during runtime.
    if (vkCreateDebugUtilsMessengerEXT != nullptr)
    {
        vkCreateDebugUtilsMessengerEXT(m_instance, &messenger_create_info, nullptr, &m_debug_messenger);
    }

    if (result<void> ret = select_physical_device(); !ret)
    {
        return ret;
    }
    if (result<void> ret = select_queue_families(); !ret)
    {
        return ret;
    }
    if (result<void> ret = check_feature_support(); !ret)
    {
        return ret;
    }
    if (result<void> ret = create_logical_device(); !ret)
    {
        return ret;
    }

    return true;
}

result<void> vulkan_render_interface::resolve_instance_functions()
{
    vkDestroyDebugUtilsMessengerEXT                 = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
    vkCreateDebugUtilsMessengerEXT                  = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));

    return true;
}

result<void> vulkan_render_interface::resolve_device_functions()
{
    vkGetAccelerationStructureBuildSizesKHR         = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR"));
    vkCreateAccelerationStructureKHR                = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR               = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR"));
    vkCmdBuildAccelerationStructuresKHR             = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR"));
    vkCmdCopyAccelerationStructureKHR               = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkCmdCopyAccelerationStructureKHR"));
    vkCmdWriteAccelerationStructuresPropertiesKHR   = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(m_device, "vkCmdWriteAccelerationStructuresPropertiesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR      = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCreateRayTracingPipelinesKHR                  = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_device, "vkCreateRayTracingPipelinesKHR"));
    vkGetRayTracingShaderGroupHandlesKHR            = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_device, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCmdTraceRaysKHR                               = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysKHR"));
    vkSetDebugUtilsObjectNameEXT                    = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(m_instance, "vkSetDebugUtilsObjectNameEXT"));

    return true;
}

result<void> vulkan_render_interface::select_physical_device()
{
    // Grab list of all evice.
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

    if (device_count == 0)
    {
        db_error(render_interface, "Failed to find any vulkan capable gpus.");
        return standard_errors::failed;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

    // Iterate through all device and score them to find the one that best matches our requirements.
    using ScoredDevice = std::pair<int64_t, VkPhysicalDevice>;
    std::vector<ScoredDevice> scored_devices;

    for (VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.apiVersion < VK_API_VERSION_1_3)
        {
            continue;
        }

        // Ensure we have all the extensions we require.
        uint32_t device_extension_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &device_extension_count, nullptr);
        m_available_physical_device_extensions.resize(device_extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &device_extension_count, m_available_physical_device_extensions.data());

        bool is_supported = true;
        for (const char* name : m_required_physical_device_extensions)
        {
            if (!check_physical_device_extension_support(name))
            {
                is_supported = false;
                break;
            }
        }
        if (!is_supported)
        {
            continue;
        }

        // Grab the memory properties and score them so we prefer discrete gpus with the
        // highest memory available.
        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);

        int64_t score = 0;

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1'000'000'000;
        }
        else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            score += 100'000'000;
        }
        else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
        {
            score -= 1'000'000'000;
        }

        for (uint32_t i = 0; i < memory_properties.memoryHeapCount; i++)
        {
            if (memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                score += static_cast<int64_t>(memory_properties.memoryHeaps[i].size / (1024 * 1024));
            }
        }

        scored_devices.push_back({ score, device });
    }

    // Check we have a valid set of devices and sort by highest score.
    if (scored_devices.empty())
    {
        db_error(render_interface, "Failed to find any gpu meeting the minimum requirements.");
        return standard_errors::failed;
    }

    std::sort(scored_devices.begin(), scored_devices.end(), [](const ScoredDevice& a, const ScoredDevice& b) {
        return a.first > b.first;
    });

    m_physical_device = scored_devices[0].second;

    // Log out the available devies.
    db_log(render_interface, "Graphics Adapters:");
    for (VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);

        size_t device_local_bytes = 0;
        size_t device_non_local_bytes = 0;
        for (uint32_t h = 0; h < memory_properties.memoryHeapCount; h++)
        {
            if (memory_properties.memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                device_local_bytes += memory_properties.memoryHeaps[h].size;
            }
            else
            {
                device_local_bytes += memory_properties.memoryHeaps[h].size;
            }
        }

        if (m_physical_device == device)
        {
            m_vram_total_local = device_local_bytes;
            m_vram_total_non_local = device_non_local_bytes;
        }

        bool is_supported = false;
        for (const ScoredDevice& scored_device : scored_devices)
        {
            if (scored_device.second == device)
            {
                is_supported = true;
            }
        }

        const char* deviceTypeName = "";
        switch (properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:             deviceTypeName = "Other";       break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:    deviceTypeName = "Integrated";  break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:      deviceTypeName = "Discrete";    break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:       deviceTypeName = "Virtual";     break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:               deviceTypeName = "CPU";         break;
        }

        db_log(render_interface, "[%c] %-40s %s", (m_physical_device == device) ? '*' : ' ', properties.deviceName, is_supported ? "" : "(Not Supported)");
        db_log(render_interface, "\tLocal Memory: %zi mb", device_local_bytes / 1024 / 1024);
        db_log(render_interface, "\tDriver Version: %u", properties.driverVersion);
        db_log(render_interface, "\tAPI Version: %u", properties.apiVersion);
        db_log(render_interface, "\tDevice Type: %s", deviceTypeName);
        db_log(render_interface, "\tVendor ID: %u", properties.vendorID);
        db_log(render_interface, "\tDevice ID: %u", properties.deviceID);
    }

    // Log out memory stats of the device selected.
    VkPhysicalDeviceMemoryProperties primary_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &primary_memory_properties);

    db_log(render_interface, "Memory Heaps:");
    for (uint32_t i = 0; i < primary_memory_properties.memoryHeapCount; i++)
    {
        VkMemoryHeap heap = primary_memory_properties.memoryHeaps[i];
        std::string flags = "";
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            flags += "local";
        }
        if (heap.flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT || heap.flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR)
        {
            if (!flags.empty()) flags += ", ";
            flags += "multi-instance";
        }
        if (heap.flags & VK_MEMORY_HEAP_TILE_MEMORY_BIT_QCOM)
        {
            if (!flags.empty()) flags += ", ";
            flags += "tile-memory";
        }
        db_log(render_interface, "\t[%i] Size=%llu mb Flags=%s ", i, heap.size / (1024 * 1024), flags.c_str());
    }

    db_log(render_interface, "Memory Types:");
    for (uint32_t i = 0; i < primary_memory_properties.memoryTypeCount; i++)
    {
        VkMemoryType type = primary_memory_properties.memoryTypes[i];
        std::string flags = "";
        if (type.propertyFlags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            flags += "local";
        }
        if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            if (!flags.empty()) flags += ", ";
            flags += "host-visible";
        }
        if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            if (!flags.empty()) flags += ", ";
            flags += "host-coherent";
        }
        if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        {
            if (!flags.empty()) flags += ", ";
            flags += "host-cached";
        }
        if (type.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
        {
            if (!flags.empty()) flags += ", ";
            flags += "lazy-allocated";
        }
        if (type.propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
        {
            if (!flags.empty()) flags += ", ";
            flags += "protected";
        }
        if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
        {
            if (!flags.empty()) flags += ", ";
            flags += "device-coherent";
        }
        if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
        {
            if (!flags.empty()) flags += ", ";
            flags += "device-uncached";
        }
        if (type.propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
        {
            if (!flags.empty()) flags += ", ";
            flags += "rdma-capable";
        }
        db_log(render_interface, "\t[%i] HeapIndex=%i Flags=%s ", i, type.heapIndex, flags.c_str());
    }

    // Log out device extensions.
    uint32_t device_extension_count = 0;
    vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &device_extension_count, nullptr);
    m_available_physical_device_extensions.resize(device_extension_count);
    vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &device_extension_count, m_available_physical_device_extensions.data());

    db_log(render_interface, "Device Extensions:");
    for (uint32_t i = 0; i < device_extension_count; i++)
    {
        db_log(render_interface, "\t%s", m_available_physical_device_extensions[i].extensionName);
    }

    return true;
}

result<void> vulkan_render_interface::select_queue_families()
{
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &family_count, nullptr);

    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &family_count, families.data());

    m_graphics_queue_family = -1;
    m_copy_queue_family = -1;

    for (uint32_t i = 0; i < family_count; i++)
    {
        const VkQueueFamilyProperties& family = families[i];

        if (m_graphics_queue_family < 0 && (family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            m_graphics_queue_family = i;
        }

        if (m_copy_queue_family < 0 &&
             (family.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(family.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(family.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            m_copy_queue_family = i;
        }
    }

    if (m_graphics_queue_family < 0)
    {
        db_error(render_interface, "Failed to find a queue family supporting graphics operations.");
        return standard_errors::failed;
    }

    if (m_copy_queue_family < 0)
    {
        m_copy_queue_family = m_graphics_queue_family;
    }

    return true;
}

result<void> vulkan_render_interface::check_feature_support()
{
    //  Query features and check we have everything we need.
    VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features = {};
    ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_pipeline_features = {};
    raytracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    raytracing_pipeline_features.pNext = &ray_query_features;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {};
    acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acceleration_structure_features.pNext = &raytracing_pipeline_features;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features = {};
    extended_dynamic_state_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extended_dynamic_state_features.pNext = &acceleration_structure_features;

    VkPhysicalDeviceVulkan13Features vulkan13_features = {};
    vulkan13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13_features.pNext = &extended_dynamic_state_features;

    VkPhysicalDeviceVulkan12Features vulkan12_features = {};
    vulkan12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12_features.pNext = &vulkan13_features;

    VkPhysicalDeviceFeatures2 features2 = {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &vulkan12_features;

    vkGetPhysicalDeviceFeatures2(m_physical_device, &features2);

    m_feature_support[(int)ri_feature::raytracing] =
        check_physical_device_extension_support(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
        check_physical_device_extension_support(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) &&
        check_physical_device_extension_support(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) &&
        check_physical_device_extension_support(VK_KHR_RAY_QUERY_EXTENSION_NAME) &&
        acceleration_structure_features.accelerationStructure &&
        acceleration_structure_features.descriptorBindingAccelerationStructureUpdateAfterBind &&
        raytracing_pipeline_features.rayTracingPipeline &&
        ray_query_features.rayQuery;

    bool bindless_supported =
        vulkan12_features.bufferDeviceAddress &&
        vulkan12_features.descriptorBindingPartiallyBound &&
        vulkan12_features.descriptorBindingVariableDescriptorCount &&
        vulkan12_features.runtimeDescriptorArray &&
        vulkan12_features.shaderSampledImageArrayNonUniformIndexing &&
        vulkan12_features.shaderStorageBufferArrayNonUniformIndexing &&
        vulkan12_features.timelineSemaphore;

    if (!bindless_supported)
    {
        db_error(render_interface, "GPU does not support the required bindless descriptor indexing / buffer device address / timeline semaphore features.");
        return standard_errors::failed;
    }

    if (!vulkan13_features.dynamicRendering || !vulkan13_features.synchronization2)
    {
        db_error(render_interface, "GPU does not support the required dynamic rendering / synchronization2 features.");
        return standard_errors::failed;
    }

    if (!extended_dynamic_state_features.extendedDynamicState)
    {
        db_error(render_interface, "GPU does not support the required extended dynamic state feature.");
        return standard_errors::failed;
    }

    db_log(render_interface, "Feature Support:");
    for (size_t i = 0; i < (int)ri_feature::COUNT; i++)
    {
        db_log(render_interface, "\t%s: %s", ri_feature_strings[i], m_feature_support[i] ? "Supported" : "Not Supported");
    }

    return true;
}

result<void> vulkan_render_interface::create_logical_device()
{
    float queue_priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = m_graphics_queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);

    if (m_copy_queue_family != m_graphics_queue_family)
    {
        queue_create_info.queueFamilyIndex = m_copy_queue_family;
        queue_create_infos.push_back(queue_create_info);
    }

    std::vector<const char*> device_extensions;
    device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    device_extensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
    if (check_feature(ri_feature::raytracing))
    {
        device_extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    }

    // Build up and create the device.
    VkPhysicalDeviceRayQueryFeaturesKHR enable_ray_query_features = {};
    enable_ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    enable_ray_query_features.rayQuery = check_feature(ri_feature::raytracing);

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enable_raytracing_pipeline_features = {};
    enable_raytracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    enable_raytracing_pipeline_features.rayTracingPipeline = check_feature(ri_feature::raytracing);
    enable_raytracing_pipeline_features.pNext = &enable_ray_query_features;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR enable_acceleration_structure_features = {};
    enable_acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    enable_acceleration_structure_features.accelerationStructure = check_feature(ri_feature::raytracing);
    enable_acceleration_structure_features.descriptorBindingAccelerationStructureUpdateAfterBind = check_feature(ri_feature::raytracing);
    enable_acceleration_structure_features.pNext = &enable_raytracing_pipeline_features;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT enable_extended_dynamic_state_features = {};
    enable_extended_dynamic_state_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    enable_extended_dynamic_state_features.extendedDynamicState = VK_TRUE;
    enable_extended_dynamic_state_features.pNext = &enable_acceleration_structure_features;

    VkPhysicalDeviceVulkan13Features enable_vulkan13_features = {};
    enable_vulkan13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    enable_vulkan13_features.dynamicRendering = VK_TRUE;
    enable_vulkan13_features.synchronization2 = VK_TRUE;
    enable_vulkan13_features.pNext = &enable_extended_dynamic_state_features;

    VkPhysicalDeviceVulkan12Features enable_vulkan12_features = {};
    enable_vulkan12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    enable_vulkan12_features.bufferDeviceAddress = VK_TRUE;
    enable_vulkan12_features.descriptorBindingPartiallyBound = VK_TRUE;
    enable_vulkan12_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    enable_vulkan12_features.runtimeDescriptorArray = VK_TRUE;
    enable_vulkan12_features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    enable_vulkan12_features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    enable_vulkan12_features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    enable_vulkan12_features.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    enable_vulkan12_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    enable_vulkan12_features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    enable_vulkan12_features.timelineSemaphore = VK_TRUE;
    enable_vulkan12_features.pNext = &enable_vulkan13_features;

    VkPhysicalDeviceFeatures2 enable_features2 = {};
    enable_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    enable_features2.pNext = &enable_vulkan12_features;
    enable_features2.features.samplerAnisotropy = VK_TRUE;
    enable_features2.features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
    enable_features2.features.sparseBinding = VK_TRUE;
    enable_features2.features.sparseResidencyImage2D = VK_TRUE;
    enable_features2.features.depthClamp = VK_TRUE;
    enable_features2.features.fillModeNonSolid = VK_TRUE;
    enable_features2.features.independentBlend = VK_TRUE;
    enable_features2.features.fragmentStoresAndAtomics = VK_TRUE;
    enable_features2.features.vertexPipelineStoresAndAtomics = VK_TRUE;
    enable_features2.features.shaderFloat64 = VK_TRUE;
    enable_features2.features.shaderInt64 = VK_TRUE;

    VkPhysicalDeviceMemoryBudgetPropertiesEXT physical_device_memory_budget_properties = {};
    physical_device_memory_budget_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
    physical_device_memory_budget_properties.pNext = &enable_features2;

    VkPhysicalDeviceMemoryProperties2 device_memory_properties = {};
    device_memory_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    device_memory_properties.pNext = &physical_device_memory_budget_properties;

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &device_memory_properties;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    VkResult vk_result = vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device);
    if (!check_result(vk_result, "vkCreateDevice"))
    {
        return standard_errors::failed;
    }

    if (result<void> ret = resolve_device_functions(); !ret)
    {
        return ret;
    }

    return true;
}

result<void> vulkan_render_interface::destroy_device()
{
    if (m_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_debug_messenger != VK_NULL_HANDLE)
    {
        if (vkDestroyDebugUtilsMessengerEXT != nullptr)
        {
            vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
        }
        m_debug_messenger = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    return true;
}

result<void> vulkan_render_interface::create_command_queues()
{
    m_graphics_queue = std::make_unique<vulkan_ri_command_queue>(*this, "Graphics Command Queue", m_graphics_queue_family);
    if (!m_graphics_queue->create_resources())
    {
        return standard_errors::failed;
    }

    m_copy_queue = std::make_unique<vulkan_ri_command_queue>(*this, "Copy Command Queue", m_copy_queue_family);
    if (!m_copy_queue->create_resources())
    {
        return standard_errors::failed;
    }

    return true;
}

result<void> vulkan_render_interface::destroy_command_queues()
{
    m_copy_queue = nullptr;
    m_graphics_queue = nullptr;
    return true;
}

result<void> vulkan_render_interface::create_heaps()
{
    // Create tables for each resource type.
    for (size_t i = 0; i < static_cast<size_t>(ri_descriptor_table::COUNT); i++)
    {
        std::unique_ptr<vulkan_ri_descriptor_table>& table = m_descriptor_tables[i];

        table = std::make_unique<vulkan_ri_descriptor_table>(*this, static_cast<ri_descriptor_table>(i));
        if (!table->create_resources())
        {
            return standard_errors::failed;
        }
    }

    return true;
}

result<void> vulkan_render_interface::destroy_heaps()
{
    for (std::unique_ptr<vulkan_ri_descriptor_table>& table : m_descriptor_tables)
    {
        table = nullptr;
    }

    return true;
}

result<void> vulkan_render_interface::create_misc()
{
    m_upload_manager = std::make_unique<vulkan_ri_upload_manager>(*this);
    if (!m_upload_manager->create_resources())
    {
        return standard_errors::failed;
    }

    m_tile_manager = std::make_unique<vulkan_ri_tile_manager>(*this);
    if (!m_tile_manager->create_resources())
    {
        return standard_errors::failed;
    }

    m_query_manager = std::make_unique<vulkan_ri_query_manager>(*this, k_maximum_queries);
    if (!m_query_manager->create_resources())
    {
        return standard_errors::failed;
    }

    m_small_buffer_allocator = std::make_unique<vulkan_ri_small_buffer_allocator>(*this);
    if (!m_small_buffer_allocator->create_resources())
    {
        return standard_errors::failed;
    }

    return true;
}

result<void> vulkan_render_interface::destroy_misc()
{
    m_small_buffer_allocator = nullptr;
    m_query_manager = nullptr;
    m_tile_manager = nullptr;
    m_upload_manager = nullptr;

    return true;
}

}; // namespace ws
