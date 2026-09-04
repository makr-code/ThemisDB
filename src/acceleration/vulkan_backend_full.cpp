/**
 * @file vulkan_backend_full.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=12; TODO=1, Stub=9, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Full Vulkan Backend Implementation for ThemisDB
// Provides GPU-accelerated vector operations using Vulkan Compute Shaders
// Cross-platform support: Windows, Linux, macOS (via MoltenVK), Android

#include "acceleration/graphics_backends.h"
#include <iostream>
#include <fstream>
#include <functional>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cstring>
#ifndef _WIN32
#include <dlfcn.h>  // For dynamic library loading (Unix)
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef THEMIS_ENABLE_VULKAN
#include <vulkan/vulkan.h>

#include <mutex>

namespace themis {
namespace acceleration {

// ── STUB #169 bridge — access storage from graphics_backends.cpp (extern) ───
// The mutex and fn are defined in graphics_backends.cpp (always-compiled TU)
// so that setCompileGLSLFn() is callable even when Vulkan is not enabled.
// Here we declare them with extern to access them from compileGLSLtoSPIRV().
namespace glsl_bridge {
    extern std::mutex                       s_vk_compile_glsl_mutex;
    extern VulkanVectorBackend::CompileGLSLFn s_vk_compile_glsl_fn;
}

// ============================================================================
// GLSL → SPIR-V compiler injection
// ============================================================================

namespace {

std::mutex     g_glsl_compiler_mutex;
GlslCompilerFn g_glsl_compiler_fn;

} // anonymous namespace

/**
 * @brief Inject a runtime GLSL-to-SPIR-V compiler (e.g. shaderc).
 *
 * When @p fn is non-null, `compileGLSLtoSPIRV()` delegates to it instead of
 * returning an empty buffer.  Pass `nullptr` to revert to the stub path.
 *
 * Roadmap ref: src/acceleration/FUTURE_ENHANCEMENTS.md §Vulkan GLSL Compiler.
 */
void setVulkanGlslCompilerFn(GlslCompilerFn fn) {
    std::lock_guard<std::mutex> lk(g_glsl_compiler_mutex);
    g_glsl_compiler_fn = std::move(fn);
}

// ============================================================================
// Vulkan Helper Structures
// ============================================================================

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    uint32_t computeQueueFamilyIndex = 0;
    
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    
    // Compute pipelines
    VkPipeline l2Pipeline = VK_NULL_HANDLE;
    VkPipeline cosinePipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    
    // Shader modules
    VkShaderModule l2ShaderModule = VK_NULL_HANDLE;
    VkShaderModule cosineShaderModule = VK_NULL_HANDLE;
    
    // Device properties
    VkPhysicalDeviceProperties deviceProps;
    VkPhysicalDeviceMemoryProperties memoryProps;
};

struct VulkanBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    void* mapped = nullptr;
};

// ============================================================================
// Vulkan Helper Functions
// ============================================================================

static bool checkValidationLayerSupport(const std::vector<const char*>& layers) {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    for (const char* layerName : layers) {
        bool found = false;
        for (const auto& layerProps : availableLayers) {
            if (strcmp(layerName, layerProps.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
          return false;
        }
    }
    return true;
}

static uint32_t findMemoryType(const VkPhysicalDeviceMemoryProperties& memProps,
                                uint32_t typeFilter,
                                VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");
}

static VkShaderModule createShaderModule(VkDevice device, const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    
    VkShaderModule shaderModule = {};
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
    
    return shaderModule;
}

[[maybe_unused]] static std::vector<uint32_t> compileGLSLtoSPIRV(
    const std::string& glslSource,
    const std::string& shaderType) {
    // Check for an injected GLSL→SPIR-V compiler first.
    {
        std::lock_guard<std::mutex> lk(g_glsl_compiler_mutex);
        if (g_glsl_compiler_fn) {
            auto spirv = g_glsl_compiler_fn(glslSource, shaderType);
            if (!spirv.empty()) {
                return spirv;
            }
        }
    }
    // STUB/SIMULATION NOTE:
    // Purpose: Stub for runtime GLSL→SPIR-V compilation while the shaderc
    //          library is not linked into ThemisDB.
    // Activation: No GlslCompilerFn injected via setVulkanGlslCompilerFn() and
    //             shaderc is not a ThemisDB build dependency.
    // Production Delta: Returns an empty SPIR-V buffer.  Any compute shader
    //                   that goes through this path fails to create a
    //                   VkShaderModule; Vulkan-accelerated vector operations
    //                   are silently disabled at pipeline creation time.
    //                   Callers must supply pre-compiled .spv files via
    //                   loadSPIRV() or inject a real compiler via
    //                   setVulkanGlslCompilerFn().
    // Removal Plan: Link libshaderc_combined and inject it via
    //               setVulkanGlslCompilerFn(); or ship pre-compiled SPIR-V assets
    //               and use loadSPIRV().  See
    //               src/acceleration/FUTURE_ENHANCEMENTS.md §Vulkan GLSL Compiler.
    (void)glslSource;
    (void)shaderType;
    std::cerr << "GLSL to SPIR-V compilation requires shaderc library (STUB)" << std::endl;
    // Check injected fn first (STUB #169 bridge).
    // Storage lives in graphics_backends.cpp; accessed here via glsl_bridge extern.
    VulkanVectorBackend::CompileGLSLFn fn_copy;
    {
        std::lock_guard<std::mutex> lk(glsl_bridge::s_vk_compile_glsl_mutex);
        fn_copy = glsl_bridge::s_vk_compile_glsl_fn;
    }
    if (fn_copy) {
        auto spv = fn_copy(glslSource, shaderType);
        if (!spv.empty()) {
            return spv;
        }
    }

    // Built-in stub path: shaderc is not a ThemisDB build dependency.
    // Inject a real compiler via VulkanVectorBackend::setCompileGLSLFn() or
    // supply pre-compiled .spv files via loadSPIRV().
    // See src/acceleration/FUTURE_ENHANCEMENTS.md §Vulkan GLSL Compiler.
    std::cerr << "GLSL to SPIR-V compilation requires shaderc library (STUB #169)" << std::endl;
    std::cerr << "Pre-compile shaders with: glslangValidator -V shader.comp -o shader.spv" << std::endl;
    return {};
}

static std::vector<uint32_t> loadSPIRV(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open SPIR-V file: " + filename);
    }
    
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    
    return buffer;
}

// ============================================================================
// VulkanVectorBackend Implementation Extension
// ============================================================================

class VulkanVectorBackendImpl {
public:
    VulkanContext ctx;
    
    bool createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "ThemisDB";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "ThemisDB Acceleration";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;
        
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        
        // Enable validation layers in debug mode
#ifdef NDEBUG
        const bool enableValidation = false;
#else
        const bool enableValidation = true;
#endif
        
        std::vector<const char*> validationLayers = {};

        if (enableValidation) {
            validationLayers.push_back("VK_LAYER_KHRONOS_validation");
            if (!checkValidationLayerSupport(validationLayers)) {
                std::cout << "Validation layers requested but not available" << std::endl;
                validationLayers.clear();
            }
        }
        
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        
        if (vkCreateInstance(&createInfo, nullptr, &ctx.instance) != VK_SUCCESS) {
            return false;
        }
        
        return true;
    }
    
    bool selectPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr);
        
        if (deviceCount == 0) {
            std::cerr << "No Vulkan-capable devices found" << std::endl;
            return false;
        }
        
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices.data());
        
        // Select the first discrete GPU, or integrated if no discrete found
        VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                selectedDevice = device;
                ctx.deviceProps = props;
                break;
            }
        }
        
        // Fallback to first device if no discrete GPU
        if (selectedDevice == VK_NULL_HANDLE) {
            selectedDevice = devices[0];
            vkGetPhysicalDeviceProperties(selectedDevice, &ctx.deviceProps);
        }
        
        ctx.physicalDevice = selectedDevice;
        vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &ctx.memoryProps);
        
        std::cout << "Selected Vulkan device: " << ctx.deviceProps.deviceName << std::endl;
        
        // Find compute queue family
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &queueFamilyCount, nullptr);
        
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &queueFamilyCount, queueFamilies.data());
        
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                ctx.computeQueueFamilyIndex = i;
                break;
            }
        }
        
        return true;
    }
    
    bool createLogicalDevice() {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = ctx.computeQueueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        
        VkPhysicalDeviceFeatures deviceFeatures{};
        
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        
        if (vkCreateDevice(ctx.physicalDevice, &createInfo, nullptr, &ctx.device) != VK_SUCCESS) {
            return false;
        }
        
        vkGetDeviceQueue(ctx.device, ctx.computeQueueFamilyIndex, 0, &ctx.computeQueue);
        
        // Create command pool
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = ctx.computeQueueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        
        if (vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &ctx.commandPool) != VK_SUCCESS) {
            return false;
        }
        
        // Create descriptor pool
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize.descriptorCount = 100; // Support up to 100 descriptor sets
        
        VkDescriptorPoolCreateInfo descPoolInfo{};
        descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descPoolInfo.poolSizeCount = 1;
        descPoolInfo.pPoolSizes = &poolSize;
        descPoolInfo.maxSets = 100;
        
        if (vkCreateDescriptorPool(ctx.device, &descPoolInfo, nullptr, &ctx.descriptorPool) != VK_SUCCESS) {
            return false;
        }
        
        return true;
    }
    
    bool createComputePipelines() {
        // Create descriptor set layout (3 storage buffers)
        VkDescriptorSetLayoutBinding bindings[3] = {};
        
        // Binding 0: Query vectors (readonly)
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        
        // Binding 1: Database vectors (readonly)
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        
        // Binding 2: Output distances (writeonly)
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 3;
        layoutInfo.pBindings = bindings;
        
        if (vkCreateDescriptorSetLayout(ctx.device, &layoutInfo, nullptr, &ctx.descriptorSetLayout) != VK_SUCCESS) {
            return false;
        }
        
        // Create pipeline layout with push constants
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(uint32_t) * 3; // numQueries, numVectors, dim
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &ctx.descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        
        if (vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, nullptr, &ctx.pipelineLayout) != VK_SUCCESS) {
            return false;
        }
        
        // Load shader modules (SPIR-V)
        // In production, these would be pre-compiled or compiled at runtime
        try {
            // Try to load pre-compiled SPIR-V binaries
            auto l2SpirV = loadSPIRV("shaders/l2_distance.spv");
            auto cosineSpirV = loadSPIRV("shaders/cosine_distance.spv");
            
            ctx.l2ShaderModule = createShaderModule(ctx.device, l2SpirV);
            ctx.cosineShaderModule = createShaderModule(ctx.device, cosineSpirV);
        } catch (const std::exception& e) {
            std::cerr << "Failed to load SPIR-V shaders: " << e.what() << std::endl;
            std::cerr << "Please compile shaders with: glslangValidator -V shader.comp -o shader.spv" << std::endl;
            return false;
        }
        
        // Create compute pipeline for L2 distance.
        // The l2_distance.comp shader exposes LOCAL_SIZE_X (constant_id=0) and
        // LOCAL_SIZE_Y (constant_id=1) as SPIR-V specialization constants so we
        // can inject the device-optimal workgroup dimensions at pipeline-creation
        // time without recompiling the shader.
        //
        // Heuristic:
        //   • maxComputeWorkGroupInvocations >= 512 → 32×16 (typical desktop GPU)
        //   • otherwise                             → 16×16 (mobile / integrated)
        //
        // Both choices keep the total invocation count (≤1024) within spec limits.
        const uint32_t maxInvocations =
            ctx.deviceProps.limits.maxComputeWorkGroupInvocations;
        const uint32_t localSizeX = (maxInvocations >= 512) ? 32 : 16;
        const uint32_t localSizeY = (maxInvocations >= 512) ? 16 : 16;

        // MapEntries map constant_id → offset in the data block.
        VkSpecializationMapEntry specMapEntries[2] = {};
        specMapEntries[0].constantID = 0;
        specMapEntries[0].offset     = 0;
        specMapEntries[0].size       = sizeof(uint32_t);
        specMapEntries[1].constantID = 1;
        specMapEntries[1].offset     = sizeof(uint32_t);
        specMapEntries[1].size       = sizeof(uint32_t);

        uint32_t specData[2] = { localSizeX, localSizeY };

        VkSpecializationInfo l2SpecInfo{};
        l2SpecInfo.mapEntryCount = 2;
        l2SpecInfo.pMapEntries   = specMapEntries;
        l2SpecInfo.dataSize      = sizeof(specData);
        l2SpecInfo.pData         = specData;

        VkPipelineShaderStageCreateInfo l2ShaderStage{};
        l2ShaderStage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l2ShaderStage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
        l2ShaderStage.module              = ctx.l2ShaderModule;
        l2ShaderStage.pName               = "main";
        l2ShaderStage.pSpecializationInfo = &l2SpecInfo;
        
        VkComputePipelineCreateInfo l2PipelineInfo{};
        l2PipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        l2PipelineInfo.stage = l2ShaderStage;
        l2PipelineInfo.layout = ctx.pipelineLayout;
        
        if (vkCreateComputePipelines(ctx.device, VK_NULL_HANDLE, 1, &l2PipelineInfo, nullptr, &ctx.l2Pipeline) != VK_SUCCESS) {
            return false;
        }
        
        // Create compute pipeline for Cosine distance
        VkPipelineShaderStageCreateInfo cosineShaderStage{};
        cosineShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        cosineShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        cosineShaderStage.module = ctx.cosineShaderModule;
        cosineShaderStage.pName = "main";
        
        VkComputePipelineCreateInfo cosinePipelineInfo{};
        cosinePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        cosinePipelineInfo.stage = cosineShaderStage;
        cosinePipelineInfo.layout = ctx.pipelineLayout;
        
        if (vkCreateComputePipelines(ctx.device, VK_NULL_HANDLE, 1, &cosinePipelineInfo, nullptr, &ctx.cosinePipeline) != VK_SUCCESS) {
            return false;
        }
        
        return true;
    }
    
    VulkanBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        VulkanBuffer buffer;
        buffer.size = size;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateBuffer(ctx.device, &bufferInfo, nullptr, &buffer.buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(ctx.device, buffer.buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(ctx.memoryProps, memRequirements.memoryTypeBits, properties);
        
        if (vkAllocateMemory(ctx.device, &allocInfo, nullptr, &buffer.memory) != VK_SUCCESS) {
            vkDestroyBuffer(ctx.device, buffer.buffer, nullptr);
            throw std::runtime_error("Failed to allocate buffer memory");
        }
        
        vkBindBufferMemory(ctx.device, buffer.buffer, buffer.memory, 0);
        
        return buffer;
    }
    
    void destroyBuffer(VulkanBuffer& buffer) {
        if (buffer.mapped) {
            vkUnmapMemory(ctx.device, buffer.memory);
            buffer.mapped = nullptr;
        }
        if (buffer.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(ctx.device, buffer.buffer, nullptr);
            buffer.buffer = VK_NULL_HANDLE;
        }
        if (buffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(ctx.device, buffer.memory, nullptr);
            buffer.memory = VK_NULL_HANDLE;
        }
    }
    
    void cleanup() {
        if (ctx.device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(ctx.device);
            
            if (ctx.l2Pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(ctx.device, ctx.l2Pipeline, nullptr);
            }
            if (ctx.cosinePipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(ctx.device, ctx.cosinePipeline, nullptr);
            }
            if (ctx.pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(ctx.device, ctx.pipelineLayout, nullptr);
            }
            if (ctx.descriptorSetLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(ctx.device, ctx.descriptorSetLayout, nullptr);
            }
            if (ctx.l2ShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(ctx.device, ctx.l2ShaderModule, nullptr);
            }
            if (ctx.cosineShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(ctx.device, ctx.cosineShaderModule, nullptr);
            }
            if (ctx.descriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(ctx.device, ctx.descriptorPool, nullptr);
            }
            if (ctx.commandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
            }
            
            vkDestroyDevice(ctx.device, nullptr);
        }
        
        if (ctx.instance != VK_NULL_HANDLE) {
            vkDestroyInstance(ctx.instance, nullptr);
        }
    }
};

} // namespace acceleration
} // namespace themis

#endif // THEMIS_ENABLE_VULKAN
