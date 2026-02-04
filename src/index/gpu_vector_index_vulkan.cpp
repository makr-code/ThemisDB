#include "index/gpu_vector_index.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include <stdexcept>

#ifdef THEMIS_ENABLE_VULKAN
#include <vulkan/vulkan.h>
#endif

namespace themis {
namespace index {

#ifdef THEMIS_ENABLE_VULKAN

// =============================================================================
// VulkanVectorIndexBackend::Impl
// =============================================================================

class VulkanVectorIndexBackend::Impl {
public:
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;
    
    // Memory management
    VkBuffer queryBuffer = VK_NULL_HANDLE;
    VkDeviceMemory queryMemory = VK_NULL_HANDLE;
    VkBuffer vectorBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vectorMemory = VK_NULL_HANDLE;
    VkBuffer resultBuffer = VK_NULL_HANDLE;
    VkDeviceMemory resultMemory = VK_NULL_HANDLE;
    
    // Compute pipelines
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline l2Pipeline = VK_NULL_HANDLE;
    VkPipeline cosinePipeline = VK_NULL_HANDLE;
    VkPipeline innerProductPipeline = VK_NULL_HANDLE;
    
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    
    int dimension = 0;
    GPUVectorIndex::Config config;
    bool initialized = false;
    
    ~Impl() {
        cleanup();
    }
    
    bool initialize(int dim, const GPUVectorIndex::Config& cfg) {
        dimension = dim;
        config = cfg;
        
        try {
            if (!createInstance()) return false;
            if (!selectPhysicalDevice()) return false;
            if (!createLogicalDevice()) return false;
            if (!createCommandPool()) return false;
            if (!createDescriptorSetLayout()) return false;
            if (!createPipelineLayout()) return false;
            // Pipelines will be created lazily when needed
            
            initialized = true;
            std::cout << "Vulkan backend initialized successfully\n";
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Vulkan initialization failed: " << e.what() << std::endl;
            cleanup();
            return false;
        }
    }
    
    void cleanup() {
        if (!initialized) return;
        
        // Wait for device to be idle
        if (device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);
        }
        
        // Destroy pipelines
        if (l2Pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, l2Pipeline, nullptr);
        }
        if (cosinePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, cosinePipeline, nullptr);
        }
        if (innerProductPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, innerProductPipeline, nullptr);
        }
        
        // Destroy pipeline layout and descriptor set layout
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
        
        // Destroy descriptor pool
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
        
        // Destroy command pool
        if (commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }
        
        // Free buffers and memory
        destroyBuffer(queryBuffer, queryMemory);
        destroyBuffer(vectorBuffer, vectorMemory);
        destroyBuffer(resultBuffer, resultMemory);
        
        // Destroy device
        if (device != VK_NULL_HANDLE) {
            vkDestroyDevice(device, nullptr);
        }
        
        // Destroy instance
        if (instance != VK_NULL_HANDLE) {
            vkDestroyInstance(instance, nullptr);
        }
        
        initialized = false;
    }
    
private:
    bool createInstance() {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "ThemisDB GPU Vector Index";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "ThemisDB";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;
        
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        
        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create Vulkan instance: " << result << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool selectPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        
        if (deviceCount == 0) {
            std::cerr << "No Vulkan-capable devices found\n";
            return false;
        }
        
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        
        // Select the first device with compute capability
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
            
            // Find compute queue
            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    physicalDevice = device;
                    queueFamilyIndex = i;
                    std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
                    return true;
                }
            }
        }
        
        std::cerr << "No device with compute capability found\n";
        return false;
    }
    
    bool createLogicalDevice() {
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        
        VkPhysicalDeviceFeatures deviceFeatures = {};
        
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.pEnabledFeatures = &deviceFeatures;
        
        VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create logical device: " << result << std::endl;
            return false;
        }
        
        vkGetDeviceQueue(device, queueFamilyIndex, 0, &computeQueue);
        return true;
    }
    
    bool createCommandPool() {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        
        VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create command pool: " << result << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool createDescriptorSetLayout() {
        // Descriptor bindings for compute shader
        VkDescriptorSetLayoutBinding bindings[3] = {};
        
        // Binding 0: Query vectors (storage buffer)
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        
        // Binding 1: Database vectors (storage buffer)
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        
        // Binding 2: Results (storage buffer)
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 3;
        layoutInfo.pBindings = bindings;
        
        VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create descriptor set layout: " << result << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool createPipelineLayout() {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(uint32_t) * 3; // numQueries, numVectors, dimension
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        
        VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create pipeline layout: " << result << std::endl;
            return false;
        }
        
        return true;
    }
    
    void destroyBuffer(VkBuffer& buffer, VkDeviceMemory& memory) {
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }
};

#else // !THEMIS_ENABLE_VULKAN

// Stub implementation when Vulkan is not available
class VulkanVectorIndexBackend::Impl {
public:
    bool initialize(int, const GPUVectorIndex::Config&) {
        std::cerr << "Vulkan support not compiled in\n";
        return false;
    }
    void cleanup() {}
};

#endif // THEMIS_ENABLE_VULKAN

// =============================================================================
// VulkanVectorIndexBackend public interface
// =============================================================================

VulkanVectorIndexBackend::VulkanVectorIndexBackend()
    : pImpl(std::make_unique<Impl>()) {
}

VulkanVectorIndexBackend::~VulkanVectorIndexBackend() = default;

bool VulkanVectorIndexBackend::initialize(int dimension, const GPUVectorIndex::Config& config) {
    return pImpl->initialize(dimension, config);
}

void VulkanVectorIndexBackend::shutdown() {
#ifdef THEMIS_ENABLE_VULKAN
    pImpl->cleanup();
#endif
}

std::vector<float> VulkanVectorIndexBackend::computeL2Distance(
    const float* queries, size_t numQueries,
    const float* vectors, size_t numVectors, size_t dim) {
    // TODO: Implement GPU computation
    (void)queries; (void)numQueries; (void)vectors; (void)numVectors; (void)dim;
    return {};
}

std::vector<float> VulkanVectorIndexBackend::computeCosineDistance(
    const float* queries, size_t numQueries,
    const float* vectors, size_t numVectors, size_t dim) {
    // TODO: Implement GPU computation
    (void)queries; (void)numQueries; (void)vectors; (void)numVectors; (void)dim;
    return {};
}

std::vector<float> VulkanVectorIndexBackend::computeInnerProduct(
    const float* queries, size_t numQueries,
    const float* vectors, size_t numVectors, size_t dim) {
    // TODO: Implement GPU computation
    (void)queries; (void)numQueries; (void)vectors; (void)numVectors; (void)dim;
    return {};
}

std::vector<std::vector<std::pair<uint32_t, float>>> VulkanVectorIndexBackend::batchSearch(
    const float* queries, size_t numQueries,
    const float* vectors, size_t numVectors,
    size_t dim, size_t k, GPUVectorIndex::DistanceMetric metric) {
    // TODO: Implement GPU batch search
    (void)queries; (void)numQueries; (void)vectors; (void)numVectors;
    (void)dim; (void)k; (void)metric;
    return {};
}

bool VulkanVectorIndexBackend::enableMultiGPU(int numDevices) {
    // TODO: Implement multi-GPU support
    (void)numDevices;
    return false;
}

void VulkanVectorIndexBackend::distributeLoad(const std::vector<float>& vectors, int deviceId) {
    // TODO: Implement load distribution
    (void)vectors; (void)deviceId;
}

} // namespace index
} // namespace themis
