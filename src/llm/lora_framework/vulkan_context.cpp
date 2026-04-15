/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vulkan_context.cpp                                 ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:12:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     490                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/lora_framework/vulkan_context.h"
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <set>

#if THEMIS_HAS_VULKAN_HEADER

namespace themis {
namespace lora {
namespace vulkan {

// Validation layers for debugging
const std::vector<const char*> VulkanContext::validation_layers_ = {
    "VK_LAYER_KHRONOS_validation"
};

// Debug callback for validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Vulkan validation: " << callback_data->pMessage << std::endl;
    }
    
    return VK_FALSE;
}

VulkanContext::VulkanContext() = default;

VulkanContext::~VulkanContext() {
    cleanup();
}

VulkanContext::VulkanContext(VulkanContext&& other) noexcept
    : instance_(other.instance_)
    , physical_device_(other.physical_device_)
    , device_(other.device_)
    , compute_queue_(other.compute_queue_)
    , command_pool_(other.command_pool_)
    , debug_messenger_(other.debug_messenger_)
    , queue_family_index_(other.queue_family_index_)
    , device_properties_(other.device_properties_)
    , memory_properties_(other.memory_properties_)
    , initialized_(other.initialized_)
    , validation_enabled_(other.validation_enabled_) {
    
    other.instance_ = VK_NULL_HANDLE;
    other.physical_device_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
    other.compute_queue_ = VK_NULL_HANDLE;
    other.command_pool_ = VK_NULL_HANDLE;
    other.debug_messenger_ = VK_NULL_HANDLE;
    other.initialized_ = false;
}

VulkanContext& VulkanContext::operator=(VulkanContext&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        instance_ = other.instance_;
        physical_device_ = other.physical_device_;
        device_ = other.device_;
        compute_queue_ = other.compute_queue_;
        command_pool_ = other.command_pool_;
        debug_messenger_ = other.debug_messenger_;
        queue_family_index_ = other.queue_family_index_;
        device_properties_ = other.device_properties_;
        memory_properties_ = other.memory_properties_;
        initialized_ = other.initialized_;
        validation_enabled_ = other.validation_enabled_;
        
        other.instance_ = VK_NULL_HANDLE;
        other.physical_device_ = VK_NULL_HANDLE;
        other.device_ = VK_NULL_HANDLE;
        other.compute_queue_ = VK_NULL_HANDLE;
        other.command_pool_ = VK_NULL_HANDLE;
        other.debug_messenger_ = VK_NULL_HANDLE;
        other.initialized_ = false;
    }
    return *this;
}

bool VulkanContext::initialize(int device_id, bool enable_validation) {
    if (initialized_) {
        return true;
    }
    
    validation_enabled_ = enable_validation;
    
    // Check validation layer support if requested
    if (validation_enabled_ && !check_validation_layer_support()) {
        std::cerr << "Validation layers requested but not available" << std::endl;
        validation_enabled_ = false;
    }
    
    // Create Vulkan instance
    if (!create_instance(validation_enabled_)) {
        cleanup();
        return false;
    }
    
    // Setup debug messenger if validation enabled
    if (validation_enabled_) {
        setup_debug_messenger();
    }
    
    // Select physical device (GPU)
    if (!select_physical_device(device_id)) {
        cleanup();
        return false;
    }
    
    // Find compute queue family
    if (!find_queue_family()) {
        cleanup();
        return false;
    }
    
    // Create logical device
    if (!create_device()) {
        cleanup();
        return false;
    }
    
    // Create command pool
    if (!create_command_pool()) {
        cleanup();
        return false;
    }
    
    initialized_ = true;
    return true;
}

void VulkanContext::cleanup() {
    if (!initialized_) {
        return;
    }
    
    // Wait for device to be idle
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
    
    // Destroy command pool
    if (command_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
    }
    
    // Destroy device
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    
    // Destroy debug messenger
    if (debug_messenger_ != VK_NULL_HANDLE && validation_enabled_) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance_, debug_messenger_, nullptr);
        }
        debug_messenger_ = VK_NULL_HANDLE;
    }
    
    // Destroy instance
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    
    initialized_ = false;
}

bool VulkanContext::is_available() {
    // Try to create a minimal instance to check Vulkan availability
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    
    VkInstance test_instance;
    VkResult result = vkCreateInstance(&create_info, nullptr, &test_instance);
    
    if (result == VK_SUCCESS) {
        vkDestroyInstance(test_instance, nullptr);
        return true;
    }
    
    return false;
}

bool VulkanContext::create_instance(bool enable_validation) {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ThemisDB LoRA Training";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "ThemisDB";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    
    // Enable validation layers if requested
    if (enable_validation) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
        
        // Add debug utils extension
        std::vector<const char*> extensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
    }
    
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanContext::select_physical_device(int device_id) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    
    if (device_count == 0) {
        std::cerr << "No Vulkan-capable GPU found" << std::endl;
        return false;
    }
    
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
    
    // If device_id is specified and valid, use it
    if (device_id >= 0 && device_id < static_cast<int>(device_count)) {
        physical_device_ = devices[device_id];
    } else {
        // Otherwise, prefer discrete GPU
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                physical_device_ = device;
                break;
            }
        }
        
        // If no discrete GPU found, use first device
        if (physical_device_ == VK_NULL_HANDLE) {
            physical_device_ = devices[0];
        }
    }
    
    // Get device properties and memory properties
    vkGetPhysicalDeviceProperties(physical_device_, &device_properties_);
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties_);
    
    std::cout << "Selected Vulkan device: " << device_properties_.deviceName << std::endl;
    
    return true;
}

bool VulkanContext::find_queue_family() {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);
    
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count,
                                              queue_families.data());
    
    // Find a queue family that supports compute operations
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queue_family_index_ = i;
            return true;
        }
    }
    
    std::cerr << "Failed to find compute queue family" << std::endl;
    return false;
}

bool VulkanContext::create_device() {
    // Specify queue priorities
    float queue_priority = 1.0f;
    
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family_index_;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    
    // Enable required device features
    VkPhysicalDeviceFeatures device_features = {};
    
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.queueCreateInfoCount = 1;
    create_info.pEnabledFeatures = &device_features;
    
    // Enable validation layers for device (deprecated but still used in some drivers)
    if (validation_enabled_) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    }
    
    VkResult result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create logical device: " << result << std::endl;
        return false;
    }
    
    // Get compute queue handle
    vkGetDeviceQueue(device_, queue_family_index_, 0, &compute_queue_);
    
    return true;
}

bool VulkanContext::create_command_pool() {
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue_family_index_;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    VkResult result = vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create command pool: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanContext::setup_debug_messenger() {
    if (!validation_enabled_) {
        return true;
    }
    
    VkDebugUtilsMessengerCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
    
    if (func != nullptr) {
        VkResult result = func(instance_, &create_info, nullptr, &debug_messenger_);
        return result == VK_SUCCESS;
    }
    
    return false;
}

VkCommandBuffer VulkanContext::allocate_command_buffer(VkCommandBufferLevel level) {
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = level;
    alloc_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    VkResult result = vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }
    
    return command_buffer;
}

void VulkanContext::free_command_buffer(VkCommandBuffer command_buffer) {
    vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);
}

VkFence VulkanContext::create_fence(bool signaled) {
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (signaled) {
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    
    VkFence fence;
    VkResult result = vkCreateFence(device_, &fence_info, nullptr, &fence);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fence");
    }
    
    return fence;
}

void VulkanContext::destroy_fence(VkFence fence) {
    vkDestroyFence(device_, fence, nullptr);
}

bool VulkanContext::wait_for_fence(VkFence fence, uint64_t timeout_ns) {
    VkResult result = vkWaitForFences(device_, 1, &fence, VK_TRUE, timeout_ns);
    return result == VK_SUCCESS;
}

void VulkanContext::reset_fence(VkFence fence) {
    vkResetFences(device_, 1, &fence);
}

int32_t VulkanContext::find_memory_type(uint32_t type_filter,
                                         VkMemoryPropertyFlags properties) const {
    for (uint32_t i = 0; i < memory_properties_.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (memory_properties_.memoryTypes[i].propertyFlags & properties) == properties) {
            return static_cast<int32_t>(i);
        }
    }
    
    return -1;
}

bool VulkanContext::check_validation_layer_support() const {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
    
    for (const char* layer_name : validation_layers_) {
        bool layer_found = false;
        
        for (const auto& layer_props : available_layers) {
            if (strcmp(layer_name, layer_props.layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        
        if (!layer_found) {
            return false;
        }
    }
    
    return true;
}

} // namespace vulkan
} // namespace lora
} // namespace themis

#endif // THEMIS_HAS_VULKAN_HEADER
