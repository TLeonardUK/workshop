// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_queue.h"
#include "workshop.render_interface.vulkan/vulkan_ri_command_list.h"
#include "workshop.render_interface.vulkan/vulkan_ri_upload_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_tile_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_query_manager.h"
#include "workshop.render_interface.vulkan/vulkan_ri_small_buffer_allocator.h"
#include "workshop.render_interface.vulkan/vulkan_ri_descriptor_table.h"
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
#include "workshop.render_interface.vulkan/vulkan_ri_param_block.h"

#include "workshop.core/containers/string.h"
#include "workshop.core/filesystem/file.h"

#include <algorithm>
#include <array>
#include <vector>
#include <cstring>
#include <optional>

namespace ws {

namespace {

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

bool is_layer_available(const char* name)
{
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);

    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());

    for (const VkLayerProperties& layer : layers)
    {
        if (strcmp(layer.layerName, name) == 0)
        {
            return true;
        }
    }

    return false;
}

bool is_instance_extension_available(const std::vector<VkExtensionProperties>& extensions, const char* name)
{
    for (const VkExtensionProperties& extension : extensions)
    {
        if (strcmp(extension.extensionName, name) == 0)
        {
            return true;
        }
    }

    return false;
}

bool is_device_extension_available(const std::vector<VkExtensionProperties>& extensions, const char* name)
{
    for (const VkExtensionProperties& extension : extensions)
    {
        if (strcmp(extension.extensionName, name) == 0)
        {
            return true;
        }
    }

    return false;
}

}; // namespace

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

result<void> vulkan_render_interface::create_device()
{
    bool should_debug = false;
#ifdef WS_DEBUG
    should_debug = true;
#endif
    if (ws::is_option_set("vulkan_debug"))
    {
        should_debug = true;
    }

    bool validation_layer_available = should_debug && is_layer_available("VK_LAYER_KHRONOS_validation");

    // Query available instance extensions.
    uint32_t available_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(available_extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions.data());

    std::vector<const char*> instance_extensions;

    unsigned int sdl_extension_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(nullptr, &sdl_extension_count, nullptr))
    {
        db_error(render_interface, "SDL_Vulkan_GetInstanceExtensions failed with error: %s", SDL_GetError());
        return standard_errors::failed;
    }

    std::vector<const char*> sdl_extensions(sdl_extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(nullptr, &sdl_extension_count, sdl_extensions.data()))
    {
        db_error(render_interface, "SDL_Vulkan_GetInstanceExtensions failed with error: %s", SDL_GetError());
        return standard_errors::failed;
    }

    for (const char* extension : sdl_extensions)
    {
        instance_extensions.push_back(extension);
    }

    bool debug_utils_available = is_instance_extension_available(available_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if (should_debug && debug_utils_available)
    {
        instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // GPU-assisted validation instruments shader code to catch hazards core validation can't see
    // statically (eg. out-of-bounds bindless resource indices, invalid descriptor accesses) - only
    // worth the runtime overhead while debugging, so it's gated behind should_debug same as the
    // base validation layer itself.
    bool validation_features_available = should_debug && validation_layer_available && is_instance_extension_available(available_extensions, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    if (validation_features_available)
    {
        instance_extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    }

    std::vector<const char*> instance_layers;
    if (validation_layer_available)
    {
        instance_layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "workshop";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "workshop";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
    instance_create_info.ppEnabledExtensionNames = instance_extensions.data();
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(instance_layers.size());
    instance_create_info.ppEnabledLayerNames = instance_layers.data();

    VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = {};
    messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messenger_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messenger_create_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messenger_create_info.pfnUserCallback = debug_messenger_callback;

    if (should_debug && debug_utils_available)
    {
        instance_create_info.pNext = &messenger_create_info;
    }

    std::array<VkValidationFeatureEnableEXT, 1> enabled_validation_features = {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    };

    VkValidationFeaturesEXT validation_features = {};
    validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validation_features.enabledValidationFeatureCount = static_cast<uint32_t>(enabled_validation_features.size());
    validation_features.pEnabledValidationFeatures = enabled_validation_features.data();

    if (validation_features_available)
    {
        validation_features.pNext = instance_create_info.pNext;
        instance_create_info.pNext = &validation_features;
    }

    VkResult vk_result = vkCreateInstance(&instance_create_info, nullptr, &m_instance);
    if (!check_result(vk_result, "vkCreateInstance"))
    {
        return standard_errors::failed;
    }

    if (should_debug && debug_utils_available)
    {
        auto create_messenger_fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));

        if (create_messenger_fn != nullptr)
        {
            create_messenger_fn(m_instance, &messenger_create_info, nullptr, &m_debug_messenger);
        }
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

    return true;
}

result<void> vulkan_render_interface::select_physical_device()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

    if (device_count == 0)
    {
        db_error(render_interface, "Failed to find any vulkan capable gpus.");
        return standard_errors::failed;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

    using ScoredDevice = std::pair<int64_t, VkPhysicalDevice>;
    std::vector<ScoredDevice> scored_devices;

    for (VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        // We require vulkan 1.3 for dynamic rendering / synchronization2 / timeline
        // semaphores as core functionality without needing a function-pointer loader.
        if (properties.apiVersion < VK_API_VERSION_1_3)
        {
            continue;
        }

        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

        if (!is_device_extension_available(extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        {
            continue;
        }

        if (!is_device_extension_available(extensions, VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME))
        {
            continue;
        }

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

    if (scored_devices.empty())
    {
        db_error(render_interface, "Failed to find any gpu meeting the minimum requirements (vulkan 1.3, swapchain support).");
        return standard_errors::failed;
    }

    std::sort(scored_devices.begin(), scored_devices.end(), [](const ScoredDevice& a, const ScoredDevice& b) {
        return a.first > b.first;
    });

    db_log(render_interface, "Graphics Adapters:");
    for (size_t i = 0; i < scored_devices.size(); i++)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(scored_devices[i].second, &properties);

        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(scored_devices[i].second, &memory_properties);

        size_t device_local_bytes = 0;
        for (uint32_t h = 0; h < memory_properties.memoryHeapCount; h++)
        {
            if (memory_properties.memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                device_local_bytes += memory_properties.memoryHeaps[h].size;
            }
        }

        db_log(render_interface, "[%c] %-40s", i == 0 ? '*' : ' ', properties.deviceName);
        db_log(render_interface, "     DeviceLocalMemory: %zi mb", device_local_bytes / 1024 / 1024);
    }

    m_physical_device = scored_devices[0].second;

    vkGetPhysicalDeviceProperties(m_physical_device, &m_physical_device_properties);
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_physical_device_memory_properties);

    size_t device_local_bytes = 0;
    size_t non_local_bytes = 0;
    for (uint32_t h = 0; h < m_physical_device_memory_properties.memoryHeapCount; h++)
    {
        if (m_physical_device_memory_properties.memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            device_local_bytes += m_physical_device_memory_properties.memoryHeaps[h].size;
        }
        else
        {
            non_local_bytes += m_physical_device_memory_properties.memoryHeaps[h].size;
        }
    }

    m_vram_total_local = device_local_bytes;
    m_vram_total_non_local = non_local_bytes;

    return true;
}

result<void> vulkan_render_interface::select_queue_families()
{
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &family_count, nullptr);

    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &family_count, families.data());

    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> dedicated_transfer_family;

    for (uint32_t i = 0; i < family_count; i++)
    {
        const VkQueueFamilyProperties& family = families[i];

        if (!graphics_family.has_value() && (family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            graphics_family = i;
        }

        // Prefer a queue family that supports transfer but *not* graphics/compute, as
        // that is most likely to be a genuinely dedicated copy engine.
        if ((family.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(family.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(family.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            dedicated_transfer_family = i;
        }
    }

    if (!graphics_family.has_value())
    {
        db_error(render_interface, "Failed to find a queue family supporting graphics operations.");
        return standard_errors::failed;
    }

    m_graphics_queue_family = graphics_family.value();
    m_copy_queue_family = dedicated_transfer_family.value_or(graphics_family.value());

    return true;
}

result<void> vulkan_render_interface::check_feature_support()
{
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extension_count, extensions.data());

    bool has_raytracing_extensions =
        is_device_extension_available(extensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
        is_device_extension_available(extensions, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) &&
        is_device_extension_available(extensions, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) &&
        is_device_extension_available(extensions, VK_KHR_RAY_QUERY_EXTENSION_NAME);

    // Lets the swapchain create its images in whatever storage format the surface actually
    // supports while still exposing them as VK_FORMAT_R8G8B8A8_SRGB views - the engine's shared
    // render target format config (see common.yaml's sdr_swapchain output target) assumes every
    // backend's swapchain is R8G8B8A8_SRGB, which not every Vulkan surface directly supports.
    m_swapchain_mutable_format_supported = is_device_extension_available(extensions, VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME);

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

    bool raytracing_supported =
        has_raytracing_extensions &&
        acceleration_structure_features.accelerationStructure &&
        acceleration_structure_features.descriptorBindingAccelerationStructureUpdateAfterBind &&
        raytracing_pipeline_features.rayTracingPipeline &&
        ray_query_features.rayQuery;

    m_feature_support[(int)ri_feature::raytracing] = raytracing_supported;

    if (raytracing_supported)
    {
        m_raytracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 properties2 = {};
        properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties2.pNext = &m_raytracing_pipeline_properties;

        vkGetPhysicalDeviceProperties2(m_physical_device, &properties2);
    }

    db_log(render_interface, "Feature Support:");
    for (size_t i = 0; i < (int)ri_feature::COUNT; i++)
    {
        db_log(render_interface, "     %s: %s", ri_feature_strings[i], m_feature_support[i] ? "Supported" : "Not Supported");
    }

    // Now actually create the logical device with the features we determined are available.
    std::vector<uint32_t> unique_queue_families = { m_graphics_queue_family };
    if (m_copy_queue_family != m_graphics_queue_family)
    {
        unique_queue_families.push_back(m_copy_queue_family);
    }

    float queue_priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    for (uint32_t family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    std::vector<const char*> device_extensions;
    device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    device_extensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);

    if (m_swapchain_mutable_format_supported)
    {
        device_extensions.push_back(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
    }

    if (raytracing_supported)
    {
        device_extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    }

    VkPhysicalDeviceRayQueryFeaturesKHR enable_ray_query_features = {};
    enable_ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    enable_ray_query_features.rayQuery = raytracing_supported;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enable_raytracing_pipeline_features = {};
    enable_raytracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    enable_raytracing_pipeline_features.rayTracingPipeline = raytracing_supported;
    enable_raytracing_pipeline_features.pNext = &enable_ray_query_features;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR enable_acceleration_structure_features = {};
    enable_acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    enable_acceleration_structure_features.accelerationStructure = raytracing_supported;
    enable_acceleration_structure_features.descriptorBindingAccelerationStructureUpdateAfterBind = raytracing_supported;
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

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &enable_features2;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    VkResult vk_result = vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device);
    if (!check_result(vk_result, "vkCreateDevice"))
    {
        return standard_errors::failed;
    }

    if (raytracing_supported)
    {
        resolve_extension_functions();
    }

    return true;
}

void vulkan_render_interface::resolve_extension_functions()
{
    vkGetAccelerationStructureBuildSizesKHR_fn = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR"));
    vkCreateAccelerationStructureKHR_fn = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR_fn = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR"));
    vkCmdBuildAccelerationStructuresKHR_fn = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR"));
    vkCmdCopyAccelerationStructureKHR_fn = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkCmdCopyAccelerationStructureKHR"));
    vkCmdWriteAccelerationStructuresPropertiesKHR_fn = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(m_device, "vkCmdWriteAccelerationStructuresPropertiesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR_fn = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCreateRayTracingPipelinesKHR_fn = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_device, "vkCreateRayTracingPipelinesKHR"));
    vkGetRayTracingShaderGroupHandlesKHR_fn = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_device, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCmdTraceRaysKHR_fn = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysKHR"));

    vkSetDebugUtilsObjectNameEXT_fn = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(m_instance, "vkSetDebugUtilsObjectNameEXT"));
}

void vulkan_render_interface::set_debug_object_name(VkObjectType type, uint64_t handle, const char* name)
{
    if (vkSetDebugUtilsObjectNameEXT_fn == nullptr || handle == 0)
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = type;
    name_info.objectHandle = handle;
    name_info.pObjectName = name;

    vkSetDebugUtilsObjectNameEXT_fn(m_device, &name_info);
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
        auto destroy_messenger_fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));

        if (destroy_messenger_fn != nullptr)
        {
            destroy_messenger_fn(m_instance, m_debug_messenger, nullptr);
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
    // Filled in as part of the bindless descriptor set work.
    for (size_t i = 0; i < static_cast<size_t>(ri_descriptor_table::COUNT); i++)
    {
        std::unique_ptr<vulkan_ri_descriptor_table>& table = m_descriptor_tables[i];

        table = std::make_unique<vulkan_ri_descriptor_table>(*this, static_cast<ri_descriptor_table>(i));
        if (!table->create_resources())
        {
            return standard_errors::failed;
        }
    }

    // Set index 0 must exist (vulkan requires dense set indices from 0), but is unused - the
    // 9 real bindless sets are fixed at indices 1-9 by the HLSL register space layout. This is
    // a single persistent instance shared by every pipeline layout - see get_dummy_set_layout.
    VkDescriptorSetLayoutCreateInfo empty_layout_create_info = {};
    empty_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    VkResult vk_result = vkCreateDescriptorSetLayout(m_device, &empty_layout_create_info, nullptr, &m_dummy_set_layout);
    if (!check_result(vk_result, "vkCreateDescriptorSetLayout"))
    {
        return standard_errors::failed;
    }

    return true;
}

result<void> vulkan_render_interface::destroy_heaps()
{
    if (m_dummy_set_layout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_device, m_dummy_set_layout, nullptr);
        m_dummy_set_layout = VK_NULL_HANDLE;
    }

    for (std::unique_ptr<vulkan_ri_descriptor_table>& table : m_descriptor_tables)
    {
        table = nullptr;
    }

    return true;
}

VkDescriptorSetLayout vulkan_render_interface::get_dummy_set_layout()
{
    return m_dummy_set_layout;
}

bool vulkan_render_interface::is_swapchain_mutable_format_supported() const
{
    return m_swapchain_mutable_format_supported;
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

void vulkan_render_interface::begin_frame()
{
    m_frame_index++;

    process_pending_deletes();

    m_graphics_queue->begin_frame();
    m_copy_queue->begin_frame();
    m_query_manager->begin_frame();
    m_tile_manager->begin_frame();
    m_upload_manager->begin_frame();

    process_as_build_requests();
}

void vulkan_render_interface::end_frame()
{
}

void vulkan_render_interface::flush_uploads()
{
    profile_marker(profile_colors::render, "flush uploads");

    if (m_flush_upload_reentry)
    {
        return;
    }

    m_flush_upload_reentry = true;

    {
        std::scoped_lock lock(m_dirty_param_block_mutex);
        for (vulkan_ri_param_block* block : m_dirty_param_blocks)
        {
            block->upload_state();
        }
        m_dirty_param_blocks.clear();
    }

    m_upload_manager->flush();

    m_flush_upload_reentry = false;
}

std::unique_ptr<ri_swapchain> vulkan_render_interface::create_swapchain(window& for_window, const char* debug_name)
{
    std::unique_ptr<vulkan_ri_swapchain> instance = std::make_unique<vulkan_ri_swapchain>(*this, for_window, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_fence> vulkan_render_interface::create_fence(const char* debug_name)
{
    std::unique_ptr<vulkan_ri_fence> instance = std::make_unique<vulkan_ri_fence>(*this, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_shader_compiler> vulkan_render_interface::create_shader_compiler()
{
    std::unique_ptr<vulkan_ri_shader_compiler> instance = std::make_unique<vulkan_ri_shader_compiler>(*this);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_texture_compiler> vulkan_render_interface::create_texture_compiler()
{
    return std::make_unique<vulkan_ri_texture_compiler>();
}

std::unique_ptr<ri_pipeline> vulkan_render_interface::create_pipeline(const ri_pipeline::create_params& params, const char* debug_name)
{
    std::unique_ptr<vulkan_ri_pipeline> instance = std::make_unique<vulkan_ri_pipeline>(*this, params, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_param_block_archetype> vulkan_render_interface::create_param_block_archetype(const ri_param_block_archetype::create_params& params, const char* debug_name)
{
    std::unique_ptr<vulkan_ri_param_block_archetype> instance = std::make_unique<vulkan_ri_param_block_archetype>(*this, params, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_texture> vulkan_render_interface::create_texture(const ri_texture::create_params& params, const char* debug_name)
{
    std::unique_ptr<vulkan_ri_texture> instance = std::make_unique<vulkan_ri_texture>(*this, debug_name, params);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_sampler> vulkan_render_interface::create_sampler(const ri_sampler::create_params& params, const char* debug_name)
{
    std::unique_ptr<vulkan_ri_sampler> instance = std::make_unique<vulkan_ri_sampler>(*this, debug_name, params);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
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
    return std::make_unique<vulkan_ri_layout_factory>(*this, layout, usage);
}

std::unique_ptr<ri_query> vulkan_render_interface::create_query(const ri_query::create_params& params, const char* debug_name)
{
    std::unique_ptr<vulkan_ri_query> instance = std::make_unique<vulkan_ri_query>(*this, debug_name, params);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_raytracing_blas> vulkan_render_interface::create_raytracing_blas(const char* debug_name)
{
    std::unique_ptr<vulkan_ri_raytracing_blas> instance = std::make_unique<vulkan_ri_raytracing_blas>(*this, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_raytracing_tlas> vulkan_render_interface::create_raytracing_tlas(const char* debug_name)
{
    std::unique_ptr<vulkan_ri_raytracing_tlas> instance = std::make_unique<vulkan_ri_raytracing_tlas>(*this, debug_name);
    if (!instance->create_resources())
    {
        return nullptr;
    }

    return instance;
}

std::unique_ptr<ri_staging_buffer> vulkan_render_interface::create_staging_buffer(const ri_staging_buffer::create_params& params, std::span<uint8_t> linear_data)
{
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
    return k_max_pipeline_depth;
}

vulkan_ri_upload_manager& vulkan_render_interface::get_upload_manager()
{
    return *m_upload_manager;
}

vulkan_ri_tile_manager& vulkan_render_interface::get_tile_manager()
{
    return *m_tile_manager;
}

vulkan_ri_query_manager& vulkan_render_interface::get_query_manager()
{
    return *m_query_manager;
}

vulkan_ri_small_buffer_allocator& vulkan_render_interface::get_small_buffer_allocator()
{
    return *m_small_buffer_allocator;
}

vulkan_ri_descriptor_table& vulkan_render_interface::get_descriptor_table(ri_descriptor_table table)
{
    return *m_descriptor_tables[static_cast<size_t>(table)];
}

size_t vulkan_render_interface::get_frame_index()
{
    return m_frame_index;
}

size_t vulkan_render_interface::get_ray_domain_count()
{
    return m_ray_domain_count;
}

size_t vulkan_render_interface::get_ray_type_count()
{
    return m_ray_type_count;
}

void vulkan_render_interface::queue_as_build(vulkan_ri_raytracing_tlas* tlas)
{
    std::scoped_lock lock(m_pending_as_build_mutex);
    m_pending_tlas_builds.insert(tlas);
}

void vulkan_render_interface::queue_as_build(vulkan_ri_raytracing_blas* blas)
{
    std::scoped_lock lock(m_pending_as_build_mutex);
    m_pending_blas_builds.insert(blas);
    m_pending_blas_compacts.erase(blas);
}

void vulkan_render_interface::dequeue_as_build(vulkan_ri_raytracing_tlas* tlas)
{
    std::scoped_lock lock(m_pending_as_build_mutex);
    m_pending_tlas_builds.erase(tlas);
}

void vulkan_render_interface::dequeue_as_build(vulkan_ri_raytracing_blas* blas)
{
    std::scoped_lock lock(m_pending_as_build_mutex);
    m_pending_blas_builds.erase(blas);
    m_pending_blas_compacts.erase(blas);
}

void vulkan_render_interface::process_as_build_requests()
{
    std::scoped_lock lock(m_pending_as_build_mutex);

    if (m_pending_blas_builds.empty() && m_pending_blas_compacts.empty() && m_pending_tlas_builds.empty())
    {
        return;
    }

    vulkan_ri_command_list& build_list = static_cast<vulkan_ri_command_list&>(m_graphics_queue->alloc_command_list());
    build_list.open();

    // Order is important, blas should be built (and compacted, if ready) before tlas - a tlas
    // build reads each instance's blas device address/data, which must already be visible.
    // Kept as a single command list/submission throughout: the acceleration-structure barriers
    // recorded within blas->build()/blas->compact()/tlas->build() only order work within their
    // own submission, so splitting these across separate vkQueueSubmit calls would leave the
    // ordering between them unspecified.

    // BLAS builds.
    std::unordered_set<vulkan_ri_raytracing_blas*> to_build = m_pending_blas_builds;
    for (vulkan_ri_raytracing_blas* blas : to_build)
    {
        blas->build(build_list);

        if (blas->is_pending_compaction())
        {
            m_pending_blas_compacts.insert(blas);
        }
    }
    m_pending_blas_builds.clear();

    // BLAS compaction, for anything ready to compact.
    std::vector<vulkan_ri_raytracing_blas*> to_compact;
    for (auto iter = m_pending_blas_compacts.begin(); iter != m_pending_blas_compacts.end(); /* empty */)
    {
        if ((*iter)->can_compact())
        {
            to_compact.push_back(*iter);
            iter = m_pending_blas_compacts.erase(iter);
        }
        else if (!(*iter)->is_pending_compaction())
        {
            iter = m_pending_blas_compacts.erase(iter);
        }
        else
        {
            iter++;
        }
    }
    for (vulkan_ri_raytracing_blas* blas : to_compact)
    {
        if (blas->can_compact())
        {
            blas->compact(build_list);
        }
    }

    // TLAS builds.
    std::unordered_set<vulkan_ri_raytracing_tlas*> to_build_tlas = m_pending_tlas_builds;
    for (vulkan_ri_raytracing_tlas* tlas : to_build_tlas)
    {
        tlas->build(build_list);
    }
    m_pending_tlas_builds.clear();

    build_list.close();

    profile_gpu_marker(*m_graphics_queue, profile_colors::gpu_view, "build/compact raytracing acceleration structures");
    m_graphics_queue->execute(build_list);
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
    // Real usage tracking requires VK_EXT_memory_budget, left as a closeout item.
    out_local = 0;
    out_non_local = 0;
}

void vulkan_render_interface::get_vram_total(size_t& out_local_total, size_t& out_non_local_total)
{
    out_local_total = m_vram_total_local;
    out_non_local_total = m_vram_total_non_local;
}

size_t vulkan_render_interface::get_cube_map_face_index(ri_cube_map_face face)
{
    std::array<size_t, 6> lookup = {
        0, // x_pos
        1, // x_neg
        2, // y_pos
        3, // y_neg
        4, // z_pos
        5, // z_neg
    };

    return lookup[static_cast<size_t>(face)];
}

bool vulkan_render_interface::check_feature(ri_feature feature)
{
    return m_feature_support[(int)feature];
}

bool vulkan_render_interface::check_result(VkResult result, const char* context)
{
    if (result != VK_SUCCESS)
    {
        db_error(render_interface, "%s failed with result %i.", context, static_cast<int>(result));
        return false;
    }
    return true;
}

void vulkan_render_interface::assert_result(VkResult result, const char* context)
{
    if (result != VK_SUCCESS)
    {
        db_fatal(render_interface, "%s failed with result %i.", context, static_cast<int>(result));
    }
}

VkInstance vulkan_render_interface::get_instance()
{
    return m_instance;
}

VkPhysicalDevice vulkan_render_interface::get_physical_device()
{
    return m_physical_device;
}

VkDevice vulkan_render_interface::get_device()
{
    return m_device;
}

const VkPhysicalDeviceProperties& vulkan_render_interface::get_physical_device_properties()
{
    return m_physical_device_properties;
}

const VkPhysicalDeviceMemoryProperties& vulkan_render_interface::get_physical_device_memory_properties()
{
    return m_physical_device_memory_properties;
}

const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& vulkan_render_interface::get_raytracing_pipeline_properties()
{
    return m_raytracing_pipeline_properties;
}

result<uint32_t> vulkan_render_interface::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    for (uint32_t i = 0; i < m_physical_device_memory_properties.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) &&
            (m_physical_device_memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    db_error(render_interface, "Failed to find a suitable memory type for filter 0x%08x with properties 0x%08x.", type_filter, properties);
    return standard_errors::failed;
}

}; // namespace ws
