/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphics_backends.cpp                              ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🔴 ALPHA                                        ║
    • Quality Score:   35.0/100                                       ║
    • Total Lines:     968                                            ║
    • Open Issues:     TODOs: 0, Stubs: 13                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 326c1184f  2026-02-21  feat(acceleration): Phase 4.1 — ShaderIntegrityVerifier S... ║
    • b2265b9b9  2026-02-21  feat(acceleration): Phase 3.3 — BackendHealthStatus + Vul... ║
    • c2782e741  2026-02-21  feat(acceleration): Phase 3.1 — Vulkan backend metrics in... ║
    • 2f0d63150  2026-02-21  feat(acceleration): implement Vulkan compute shader pipel... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🚧 Early Development                                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "acceleration/graphics_backends.h"
#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include "acceleration/shader_integrity.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <memory>
#include <queue>
#include <stdexcept>
#include <cstring>
#include <chrono>

#ifdef THEMIS_ENABLE_VULKAN
#include <vulkan/vulkan.h>
#endif

namespace themis {
namespace acceleration {

// ============================================================================
// VulkanVectorBackend::VulkanVectorBackendImpl — full Vulkan compute pipeline
// ============================================================================

#ifdef THEMIS_ENABLE_VULKAN

class VulkanVectorBackend::VulkanVectorBackendImpl {
public:
    // Vulkan handles
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    uint32_t computeQueueFamilyIndex = 0;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Compute pipelines (L2 squared distance, cosine distance, inner product)
    VkPipeline l2Pipeline = VK_NULL_HANDLE;
    VkPipeline cosinePipeline = VK_NULL_HANDLE;
    VkPipeline innerProductPipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    VkShaderModule l2ShaderModule = VK_NULL_HANDLE;
    VkShaderModule cosineShaderModule = VK_NULL_HANDLE;
    VkShaderModule innerProductShaderModule = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties deviceProps{};
    VkPhysicalDeviceMemoryProperties memoryProps{};
    std::string vendorName;  // Human-readable vendor (e.g. "AMD", "Intel", "NVIDIA", "ARM")

    // ---- Buffer helper ------------------------------------------------
    struct BufMem {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
    };

    BufMem createBuffer(VkDeviceSize sz, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags props) {
        BufMem bm;
        bm.size = sz;

        VkBufferCreateInfo bi{};
        bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size        = sz;
        bi.usage       = usage;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bi, nullptr, &bm.buffer) != VK_SUCCESS)
            throw std::runtime_error("vkCreateBuffer failed");

        VkMemoryRequirements mr;
        vkGetBufferMemoryRequirements(device, bm.buffer, &mr);

        VkMemoryAllocateInfo ai{};
        ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize  = mr.size;
        ai.memoryTypeIndex = findMemoryType(mr.memoryTypeBits, props);
        if (vkAllocateMemory(device, &ai, nullptr, &bm.memory) != VK_SUCCESS) {
            vkDestroyBuffer(device, bm.buffer, nullptr);
            throw std::runtime_error("vkAllocateMemory failed");
        }
        vkBindBufferMemory(device, bm.buffer, bm.memory, 0);
        return bm;
    }

    void destroyBuffer(BufMem& bm) {
        if (bm.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, bm.buffer, nullptr);
            bm.buffer = VK_NULL_HANDLE;
        }
        if (bm.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, bm.memory, nullptr);
            bm.memory = VK_NULL_HANDLE;
        }
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags) {
        for (uint32_t i = 0; i < memoryProps.memoryTypeCount; ++i) {
            if ((typeFilter & (1u << i)) &&
                (memoryProps.memoryTypes[i].propertyFlags & flags) == flags)
                return i;
        }
        throw std::runtime_error("findMemoryType: no suitable type");
    }

    // ---- Shader module ------------------------------------------------
    VkShaderModule createShaderModule(const std::vector<uint32_t>& spv) {
        VkShaderModuleCreateInfo ci{};
        ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = spv.size() * sizeof(uint32_t);
        ci.pCode    = spv.data();
        VkShaderModule mod;
        if (vkCreateShaderModule(device, &ci, nullptr, &mod) != VK_SUCCESS)
            throw std::runtime_error("vkCreateShaderModule failed");
        return mod;
    }

    static std::vector<uint32_t> loadSPIRV(const std::string& path) {
        std::ifstream f(path, std::ios::ate | std::ios::binary);
        if (!f.is_open())
            throw std::runtime_error("Cannot open SPIR-V: " + path);
        size_t sz = static_cast<size_t>(f.tellg());
        std::vector<uint32_t> buf(sz / sizeof(uint32_t));
        f.seekg(0);
        f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));

        // Phase 4.1: verify shader integrity before handing bytes to Vulkan
        // Extract just the filename as the registry key
        std::string name = path;
        auto slash = path.rfind('/');
        if (slash == std::string::npos) slash = path.rfind('\\');
        if (slash != std::string::npos) name = path.substr(slash + 1);

        auto result = ShaderIntegrityVerifier::instance().verify(name, buf);
        if (!result.passed) {
            throw std::runtime_error("[ShaderIntegrity] " + result.message);
        }
        if (!result.expectedHash.empty()) {
            std::cout << "[ShaderIntegrity] " << result.message << std::endl;
        }
        return buf;
    }

    // ---- Lifecycle ----------------------------------------------------
    bool createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "ThemisDB";
        appInfo.apiVersion       = VK_API_VERSION_1_2;

        VkInstanceCreateInfo ci{};
        ci.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pApplicationInfo = &appInfo;
        return vkCreateInstance(&ci, nullptr, &instance) == VK_SUCCESS;
    }

    bool selectPhysicalDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        if (count == 0) return false;

        std::vector<VkPhysicalDevice> devs(count);
        vkEnumeratePhysicalDevices(instance, &count, devs.data());

        // Prefer a non-NVIDIA discrete GPU so that Vulkan acts as the primary
        // fallback for AMD, Intel, ARM, and other non-CUDA hardware.
        // When only NVIDIA discrete GPUs are present (CUDA is the better choice),
        // we still accept them rather than fail.
        // Selection priority: non-NVIDIA discrete > NVIDIA discrete > any integrated > first device.

        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties bestProps{};
        int bestScore = -1;

        for (const auto& d : devs) {
            VkPhysicalDeviceProperties p;
            vkGetPhysicalDeviceProperties(d, &p);

            int score = 0;
            if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score = (p.vendorID != vendor_id::NVIDIA) ? 30 : 20;
            } else if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                score = (p.vendorID != vendor_id::NVIDIA) ? 12 : 10;
            } else if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
                score = 5;
            } else {
                score = 1; // CPU or other
            }

            if (score > bestScore) {
                bestScore  = score;
                bestDevice = d;
                bestProps  = p;
            }
        }

        if (bestDevice == VK_NULL_HANDLE) {
            bestDevice = devs[0];
            vkGetPhysicalDeviceProperties(bestDevice, &bestProps);
        }

        physicalDevice = bestDevice;
        deviceProps    = bestProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProps);

        // Resolve human-readable vendor name from vendorID
        switch (deviceProps.vendorID) {
            case vendor_id::NVIDIA:   vendorName = "NVIDIA";   break;
            case vendor_id::AMD:      vendorName = "AMD";      break;
            case vendor_id::INTEL:    vendorName = "Intel";    break;
            case vendor_id::ARM:      vendorName = "ARM";      break;
            case vendor_id::QUALCOMM: vendorName = "Qualcomm"; break;
            case vendor_id::IMGTEC:   vendorName = "ImgTec";   break;
            default:                  vendorName = "Unknown";  break;
        }

        // Find compute queue family
        uint32_t qfCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfCount, nullptr);
        std::vector<VkQueueFamilyProperties> qfProps(qfCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfCount, qfProps.data());
        for (uint32_t i = 0; i < qfCount; ++i) {
            if (qfProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                computeQueueFamilyIndex = i;
                return true;
            }
        }
        return false; // no compute queue found
    }

    bool createLogicalDevice() {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo qci{};
        qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = computeQueueFamilyIndex;
        qci.queueCount       = 1;
        qci.pQueuePriorities = &priority;

        VkPhysicalDeviceFeatures features{};
        VkDeviceCreateInfo dci{};
        dci.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dci.pQueueCreateInfos    = &qci;
        dci.queueCreateInfoCount = 1;
        dci.pEnabledFeatures     = &features;
        if (vkCreateDevice(physicalDevice, &dci, nullptr, &device) != VK_SUCCESS)
            return false;

        vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);

        VkCommandPoolCreateInfo pci{};
        pci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pci.queueFamilyIndex = computeQueueFamilyIndex;
        pci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(device, &pci, nullptr, &commandPool) != VK_SUCCESS)
            return false;

        VkDescriptorPoolSize dps{};
        dps.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        dps.descriptorCount = 300; // 3 bindings × up to 100 concurrent sets

        VkDescriptorPoolCreateInfo dpci{};
        dpci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        dpci.poolSizeCount = 1;
        dpci.pPoolSizes    = &dps;
        dpci.maxSets       = 100;
        dpci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        return vkCreateDescriptorPool(device, &dpci, nullptr, &descriptorPool) == VK_SUCCESS;
    }

    bool createComputePipelines(const std::string& shaderDir) {
        // Descriptor set layout: 3 storage buffers (query, vector, output)
        VkDescriptorSetLayoutBinding bindings[3]{};
        for (uint32_t i = 0; i < 3; ++i) {
            bindings[i].binding         = i;
            bindings[i].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        VkDescriptorSetLayoutCreateInfo dslci{};
        dslci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dslci.bindingCount = 3;
        dslci.pBindings    = bindings;
        if (vkCreateDescriptorSetLayout(device, &dslci, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            return false;

        // Push constants: numQueries, numVectors, dim (3 × uint32)
        VkPushConstantRange pcr{};
        pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pcr.offset     = 0;
        pcr.size       = sizeof(uint32_t) * 3;

        VkPipelineLayoutCreateInfo plci{};
        plci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.setLayoutCount         = 1;
        plci.pSetLayouts            = &descriptorSetLayout;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges    = &pcr;
        if (vkCreatePipelineLayout(device, &plci, nullptr, &pipelineLayout) != VK_SUCCESS)
            return false;

        // Load pre-compiled SPIR-V shaders
        try {
            auto l2spv  = loadSPIRV(shaderDir + "/l2_distance.comp.spv");
            auto cosSpv = loadSPIRV(shaderDir + "/cosine_distance.comp.spv");
            auto ipSpv  = loadSPIRV(shaderDir + "/inner_product_distance.comp.spv");
            l2ShaderModule           = createShaderModule(l2spv);
            cosineShaderModule       = createShaderModule(cosSpv);
            innerProductShaderModule = createShaderModule(ipSpv);
        } catch (const std::exception& e) {
            std::cerr << "[Vulkan] Shader load failed: " << e.what()
                      << " – compile with: glslc shader.comp -o shader.spv" << std::endl;
            return false;
        }

        auto makePipeline = [&](VkShaderModule mod, VkPipeline& out) -> bool {
            VkPipelineShaderStageCreateInfo ssi{};
            ssi.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            ssi.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
            ssi.module = mod;
            ssi.pName  = "main";

            VkComputePipelineCreateInfo pci{};
            pci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pci.stage  = ssi;
            pci.layout = pipelineLayout;
            return vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr, &out) == VK_SUCCESS;
        };

        return makePipeline(l2ShaderModule, l2Pipeline) &&
               makePipeline(cosineShaderModule, cosinePipeline) &&
               makePipeline(innerProductShaderModule, innerProductPipeline);
    }

    // ---- Compute dispatch ---------------------------------------------
    std::vector<float> dispatch(const float* queries, uint32_t nq,
                                const float* vectors, uint32_t nv,
                                uint32_t dim, DistanceMetric metric) {
        const VkDeviceSize qSize  = static_cast<VkDeviceSize>(nq) * dim * sizeof(float);
        const VkDeviceSize vSize  = static_cast<VkDeviceSize>(nv) * dim * sizeof(float);
        const VkDeviceSize outSz  = static_cast<VkDeviceSize>(nq) * nv * sizeof(float);

        const VkBufferUsageFlags devUsage =
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags devProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const VkMemoryPropertyFlags hostProps =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        // Staging buffers (CPU-visible)
        auto stagQ   = createBuffer(qSize,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, hostProps);
        auto stagV   = createBuffer(vSize,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, hostProps);
        auto stagOut = createBuffer(outSz,  VK_BUFFER_USAGE_TRANSFER_DST_BIT, hostProps);

        // Device-local buffers (GPU)
        auto devQ   = createBuffer(qSize,  devUsage, devProps);
        auto devV   = createBuffer(vSize,  devUsage, devProps);
        auto devOut = createBuffer(outSz,  devUsage, devProps);

        // Copy host data into staging
        auto copyToStaging = [&](BufMem& bm, const void* src, VkDeviceSize sz) {
            void* ptr;
            vkMapMemory(device, bm.memory, 0, sz, 0, &ptr);
            std::memcpy(ptr, src, sz);
            vkUnmapMemory(device, bm.memory);
        };
        copyToStaging(stagQ, queries, qSize);
        copyToStaging(stagV, vectors, vSize);

        // Allocate and record command buffer
        VkCommandBufferAllocateInfo cbai{};
        cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.commandPool        = commandPool;
        cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbai.commandBufferCount = 1;
        VkCommandBuffer cb;
        vkAllocateCommandBuffers(device, &cbai, &cb);

        VkCommandBufferBeginInfo cbbi{};
        cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &cbbi);

        // Transfer: staging → device
        auto recordCopy = [&](BufMem& src, BufMem& dst, VkDeviceSize sz) {
            VkBufferCopy region{0, 0, sz};
            vkCmdCopyBuffer(cb, src.buffer, dst.buffer, 1, &region);
        };
        recordCopy(stagQ, devQ, qSize);
        recordCopy(stagV, devV, vSize);

        // Pipeline barrier: transfer write → compute read
        VkMemoryBarrier mb{};
        mb.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        mb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cb,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &mb, 0, nullptr, 0, nullptr);

        // Descriptor set for this dispatch
        VkDescriptorSetAllocateInfo dsai{};
        dsai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsai.descriptorPool     = descriptorPool;
        dsai.descriptorSetCount = 1;
        dsai.pSetLayouts        = &descriptorSetLayout;
        VkDescriptorSet ds;
        vkAllocateDescriptorSets(device, &dsai, &ds);

        auto makeDescBufInfo = [](VkBuffer buf, VkDeviceSize sz) {
            VkDescriptorBufferInfo dbi{};
            dbi.buffer = buf;
            dbi.offset = 0;
            dbi.range  = sz;
            return dbi;
        };
        VkDescriptorBufferInfo dbi[3] = {
            makeDescBufInfo(devQ.buffer,   qSize),
            makeDescBufInfo(devV.buffer,   vSize),
            makeDescBufInfo(devOut.buffer, outSz)
        };
        VkWriteDescriptorSet writes[3]{};
        for (uint32_t i = 0; i < 3; ++i) {
            writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet          = ds;
            writes[i].dstBinding      = i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[i].pBufferInfo     = &dbi[i];
        }
        vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);

        // Bind pipeline and dispatch — select based on distance metric
        VkPipeline pipeline;
        switch (metric) {
            case DistanceMetric::COSINE:        pipeline = cosinePipeline;       break;
            case DistanceMetric::INNER_PRODUCT: pipeline = innerProductPipeline; break;
            default:                            pipeline = l2Pipeline;           break;
        }
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE,
                                pipelineLayout, 0, 1, &ds, 0, nullptr);

        uint32_t pc[3] = {nq, nv, dim};
        vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(pc), pc);

        // Dispatch: one thread per (vector, query) pair
        // Shader uses local_size_x=16, local_size_y=16
        constexpr uint32_t LOCAL = 16;
        uint32_t gx = (nv + LOCAL - 1) / LOCAL;
        uint32_t gy = (nq + LOCAL - 1) / LOCAL;
        vkCmdDispatch(cb, gx, gy, 1);

        // Barrier: compute write → transfer read
        mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        mb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(cb,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 1, &mb, 0, nullptr, 0, nullptr);

        // Copy results: device → staging
        VkBufferCopy outRegion{0, 0, outSz};
        vkCmdCopyBuffer(cb, devOut.buffer, stagOut.buffer, 1, &outRegion);

        vkEndCommandBuffer(cb);

        // Submit and wait
        VkFenceCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        vkCreateFence(device, &fci, nullptr, &fence);

        VkSubmitInfo si{};
        si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers    = &cb;
        vkQueueSubmit(computeQueue, 1, &si, fence);
        vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

        // Read results
        std::vector<float> results(static_cast<size_t>(nq) * nv);
        {
            void* ptr;
            vkMapMemory(device, stagOut.memory, 0, outSz, 0, &ptr);
            std::memcpy(results.data(), ptr, outSz);
            vkUnmapMemory(device, stagOut.memory);
        }

        // Cleanup per-dispatch resources
        vkDestroyFence(device, fence, nullptr);
        vkFreeCommandBuffers(device, commandPool, 1, &cb);
        vkFreeDescriptorSets(device, descriptorPool, 1, &ds);

        destroyBuffer(stagQ);
        destroyBuffer(stagV);
        destroyBuffer(stagOut);
        destroyBuffer(devQ);
        destroyBuffer(devV);
        destroyBuffer(devOut);

        return results;
    }

    // ---- Cleanup ------------------------------------------------------
    void cleanup() {
        if (device == VK_NULL_HANDLE) return;
        vkDeviceWaitIdle(device);

        if (l2Pipeline != VK_NULL_HANDLE)             vkDestroyPipeline(device, l2Pipeline, nullptr);
        if (cosinePipeline != VK_NULL_HANDLE)         vkDestroyPipeline(device, cosinePipeline, nullptr);
        if (innerProductPipeline != VK_NULL_HANDLE)   vkDestroyPipeline(device, innerProductPipeline, nullptr);
        if (pipelineLayout != VK_NULL_HANDLE)         vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (descriptorSetLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        if (l2ShaderModule != VK_NULL_HANDLE)             vkDestroyShaderModule(device, l2ShaderModule, nullptr);
        if (cosineShaderModule != VK_NULL_HANDLE)         vkDestroyShaderModule(device, cosineShaderModule, nullptr);
        if (innerProductShaderModule != VK_NULL_HANDLE)   vkDestroyShaderModule(device, innerProductShaderModule, nullptr);
        if (descriptorPool != VK_NULL_HANDLE)  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        if (commandPool != VK_NULL_HANDLE)     vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;

        if (instance != VK_NULL_HANDLE) {
            vkDestroyInstance(instance, nullptr);
            instance = VK_NULL_HANDLE;
        }
    }
};

#else // !THEMIS_ENABLE_VULKAN

class VulkanVectorBackend::VulkanVectorBackendImpl {
    // Empty placeholder when Vulkan is not compiled in
};

#endif // THEMIS_ENABLE_VULKAN

// ============================================================================
// DirectX Vector Backend Stub
// ============================================================================

DirectXVectorBackend::~DirectXVectorBackend() {
    shutdown();
}

bool DirectXVectorBackend::isAvailable() const noexcept {
#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)
    // Check if DirectX 12 is available
    // Would use D3D12GetDebugInterface() or similar
    return false; // Stub: not fully implemented yet
#else
    return false;
#endif
}

BackendCapabilities DirectXVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)
    caps.supportsVectorOps = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync = true;
    caps.deviceName = "DirectX 12 (Stub)";
#endif
    return caps;
}

bool DirectXVectorBackend::initialize() {
#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)
    // Initialize DirectX 12 device and command queue
    initialized_ = false; // Stub
    return initialized_;
#else
    return false;
#endif
}

void DirectXVectorBackend::shutdown() {
#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)
    if (initialized_) {
        // Cleanup DirectX resources
        initialized_ = false;
    }
#endif
}

std::vector<float> DirectXVectorBackend::computeDistances(
    const float* /*queries*/,
    size_t /*numQueries*/,
    size_t /*dim*/,
    const float* /*vectors*/,
    size_t /*numVectors*/,
    bool /*useL2*/
) {
    return {}; // Stub
}

std::vector<std::vector<std::pair<uint32_t, float>>> DirectXVectorBackend::batchKnnSearch(
    const float* /*queries*/,
    size_t /*numQueries*/,
    size_t /*dim*/,
    const float* /*vectors*/,
    size_t /*numVectors*/,
    size_t /*k*/,
    bool /*useL2*/
) {
    return {}; // Stub
}

// ============================================================================
// VulkanVectorBackend — public interface implementation
// ============================================================================

VulkanVectorBackend::VulkanVectorBackend()
    : initialized_(false), impl_(std::make_unique<VulkanVectorBackendImpl>()) {}

VulkanVectorBackend::~VulkanVectorBackend() {
    shutdown();
}

bool VulkanVectorBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_VULKAN
    // Probe Vulkan availability by attempting a minimal instance creation
    VkApplicationInfo appInfo{};
    appInfo.sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    VkInstance probe = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&ci, nullptr, &probe);
    if (result == VK_SUCCESS) {
        vkDestroyInstance(probe, nullptr);
        return true;
    }
    return false;
#else
    return false;
#endif
}

BackendCapabilities VulkanVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_VULKAN
    caps.supportsVectorOps       = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;
    caps.supportedMetrics        = metricBit(DistanceMetric::L2)
                                 | metricBit(DistanceMetric::COSINE)
                                 | metricBit(DistanceMetric::INNER_PRODUCT);

    if (initialized_ && impl_ && impl_->device != VK_NULL_HANDLE) {
        caps.deviceName    = std::string(impl_->deviceProps.deviceName);
        caps.vendorName    = impl_->vendorName;
        caps.computeUnits  = static_cast<int>(
            impl_->deviceProps.limits.maxComputeWorkGroupCount[0]);
        // Report device-local heap size
        for (uint32_t i = 0; i < impl_->memoryProps.memoryHeapCount; ++i) {
            if (impl_->memoryProps.memoryHeaps[i].flags &
                VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                caps.maxMemoryBytes = impl_->memoryProps.memoryHeaps[i].size;
                break;
            }
        }
    } else {
        caps.deviceName   = "Vulkan Compute";
        caps.computeUnits = 1;
        caps.maxMemoryBytes = 0;
    }
#endif
    return caps;
}

bool VulkanVectorBackend::initialize() {
#ifdef THEMIS_ENABLE_VULKAN
    if (initialized_) return true;

    auto initStart = std::chrono::steady_clock::now();

    if (!impl_) impl_ = std::make_unique<VulkanVectorBackendImpl>();

    if (!impl_->createInstance()) {
        std::cerr << "[Vulkan] vkCreateInstance failed – no Vulkan ICD?" << std::endl;
        metrics_.recordInitFailure();
        metrics_.recordError("vkCreateInstance_failed");
        return false;
    }
    if (!impl_->selectPhysicalDevice()) {
        std::cerr << "[Vulkan] No suitable physical device found" << std::endl;
        impl_->cleanup();
        metrics_.recordInitFailure();
        metrics_.recordError("no_physical_device");
        return false;
    }
    if (!impl_->createLogicalDevice()) {
        std::cerr << "[Vulkan] vkCreateDevice failed" << std::endl;
        impl_->cleanup();
        metrics_.recordInitFailure();
        metrics_.recordError("vkCreateDevice_failed");
        return false;
    }

    // Attempt to find compiled shaders in common locations
    const std::vector<std::string> shaderDirs = {
        "shaders/vector_index",
        "../shaders/vector_index",
        "./shaders",
    };
    bool pipelinesCreated = false;
    for (const auto& dir : shaderDirs) {
        if (impl_->createComputePipelines(dir)) {
            pipelinesCreated = true;
            break;
        }
    }
    if (!pipelinesCreated) {
        std::cerr << "[Vulkan] Compute pipelines unavailable – "
                     "compile shaders with: glslc shader.comp -o shader.spv" << std::endl;
        impl_->cleanup();
        metrics_.recordInitFailure();
        metrics_.recordError("shader_load_failed");
        return false;
    }

    double initSeconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - initStart).count();

    std::cout << "[Vulkan] Initialized: " << impl_->deviceProps.deviceName << std::endl;
    initialized_ = true;
    clearError();

    metrics_.recordInitSuccess();
    metrics_.recordInitDuration(initSeconds);
    metrics_.setDeviceCount(1);
    metrics_.setActiveDeviceIndex(0);

    // Expose device-local memory to metrics
    for (uint32_t i = 0; i < impl_->memoryProps.memoryHeapCount; ++i) {
        if (impl_->memoryProps.memoryHeaps[i].flags &
            VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            metrics_.setDeviceMemoryAvailable(
                static_cast<double>(impl_->memoryProps.memoryHeaps[i].size));
            break;
        }
    }

    return true;
#else
    return false;
#endif
}

void VulkanVectorBackend::shutdown() {
#ifdef THEMIS_ENABLE_VULKAN
    if (initialized_ && impl_) {
        impl_->cleanup();
        initialized_ = false;
    }
#endif
}

BackendHealthStatus VulkanVectorBackend::getHealthStatus() const {
#ifdef THEMIS_ENABLE_VULKAN
    if (!initialized_ || !impl_) {
        // Check if Vulkan ICD is reachable at all
        if (!isAvailable()) {
            return BackendHealthStatus::makeUnhealthy(
                "Vulkan ICD not available on this system");
        }
        // Available but not yet initialised (or failed init)
        const auto& err = getLastError();
        if (!err.isSuccess()) {
            return BackendHealthStatus::makeDegraded(
                "Vulkan initialization failed: " + err.message);
        }
        BackendHealthStatus s;
        s.status  = "degraded";
        s.alive   = true;
        s.healthy = false;
        s.ready   = false;
        s.message = "Vulkan backend not initialized";
        s.issues.push_back("Call initialize() before use");
        return s;
    }

    BackendHealthStatus s = BackendHealthStatus::makeHealthy(
        std::string(impl_->deviceProps.deviceName));

    s.alive = true;
    s.ready = (impl_->l2Pipeline != VK_NULL_HANDLE &&
               impl_->cosinePipeline != VK_NULL_HANDLE &&
               impl_->innerProductPipeline != VK_NULL_HANDLE);

    if (!s.ready) {
        s.status  = "degraded";
        s.healthy = false;
        s.message = "Vulkan backend initialized but compute pipelines not loaded";
        s.issues.push_back("Compile SPIR-V shaders: glslc shader.comp -o shader.spv");
    }

    // Report memory snapshot
    for (uint32_t i = 0; i < impl_->memoryProps.memoryHeapCount; ++i) {
        if (impl_->memoryProps.memoryHeaps[i].flags &
            VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            s.memoryAvailableBytes =
                static_cast<size_t>(impl_->memoryProps.memoryHeaps[i].size);
            break;
        }
    }

    s.driverInfo = std::string("Vulkan API ")
        + std::to_string(VK_API_VERSION_MAJOR(impl_->deviceProps.apiVersion)) + "."
        + std::to_string(VK_API_VERSION_MINOR(impl_->deviceProps.apiVersion)) + "."
        + std::to_string(VK_API_VERSION_PATCH(impl_->deviceProps.apiVersion));

    return s;
#else
    return BackendHealthStatus::makeUnhealthy(
        "Vulkan support not compiled in (build with THEMIS_ENABLE_VULKAN=ON)");
#endif
}

std::vector<float> VulkanVectorBackend::computeDistances(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    bool useL2  // true → L2, false → cosine (inner-product available via populateANNDispatch())
) {
#ifdef THEMIS_ENABLE_VULKAN
    if (!initialized_ || !impl_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "Vulkan",
            "Vulkan backend not initialized",
            "Call initialize() before using the backend"));
        std::cerr << "[Vulkan] computeDistances: backend not initialized" << std::endl;
        metrics_.recordError("not_initialized");
        return {};
    }
    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "Vulkan", AccelerationErrorCode::InvalidInputShape,
            "queries and vectors pointers must be non-null"));
        metrics_.recordError("null_input");
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0) {
        setError(ErrorContextHelpers::createValidationError(
            "Vulkan", AccelerationErrorCode::InvalidInputShape,
            "numQueries, numVectors, and dim must all be > 0"));
        metrics_.recordError("zero_dimension");
        return {};
    }
    auto opStart = std::chrono::steady_clock::now();
    try {
        const DistanceMetric metric = useL2 ? DistanceMetric::L2 : DistanceMetric::COSINE;
        auto result = impl_->dispatch(queries,
                                      static_cast<uint32_t>(numQueries),
                                      vectors,
                                      static_cast<uint32_t>(numVectors),
                                      static_cast<uint32_t>(dim),
                                      metric);
        double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - opStart).count();
        if (useL2) {
            metrics_.recordL2DistanceOperation(elapsed, numQueries * numVectors);
        } else {
            metrics_.recordCosineOperation(elapsed, numQueries * numVectors);
        }
        clearError();
        return result;
    } catch (const std::exception& e) {
        std::cerr << "[Vulkan] computeDistances error: " << e.what() << std::endl;
        setError(ErrorContext(
            AccelerationErrorCode::AllocationFailed,
            "Vulkan",
            std::string("Dispatch failed: ") + e.what(),
            "Reduce batch size or check GPU memory"));
        metrics_.recordError("dispatch_failed");
        metrics_.recordKernelLaunchFailure();
        return {};
    }
#else
    return {};
#endif
}

std::vector<std::vector<std::pair<uint32_t, float>>> VulkanVectorBackend::batchKnnSearch(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    size_t k,
    bool useL2
) {
#ifdef THEMIS_ENABLE_VULKAN
    if (!initialized_ || !impl_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "Vulkan",
            "Vulkan backend not initialized",
            "Call initialize() before using the backend"));
        std::cerr << "[Vulkan] batchKnnSearch: backend not initialized" << std::endl;
        metrics_.recordError("not_initialized");
        return {};
    }
    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "Vulkan", AccelerationErrorCode::InvalidInputShape,
            "queries and vectors pointers must be non-null"));
        metrics_.recordError("null_input");
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0 || k == 0) {
        setError(ErrorContextHelpers::createValidationError(
            "Vulkan", AccelerationErrorCode::InvalidInputShape,
            "numQueries, numVectors, dim, and k must all be > 0"));
        metrics_.recordError("zero_dimension");
        return {};
    }

    // Compute all pairwise distances on the GPU
    auto opStart = std::chrono::steady_clock::now();
    std::vector<float> distances;
    try {
        const DistanceMetric metric = useL2 ? DistanceMetric::L2 : DistanceMetric::COSINE;
        distances = impl_->dispatch(queries,
                                    static_cast<uint32_t>(numQueries),
                                    vectors,
                                    static_cast<uint32_t>(numVectors),
                                    static_cast<uint32_t>(dim),
                                    metric);
        double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - opStart).count();
        if (useL2) {
            metrics_.recordL2DistanceOperation(elapsed, numQueries * numVectors);
        } else {
            metrics_.recordCosineOperation(elapsed, numQueries * numVectors);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Vulkan] batchKnnSearch dispatch error: " << e.what() << std::endl;
        setError(ErrorContext(
            AccelerationErrorCode::AllocationFailed,
            "Vulkan",
            std::string("Dispatch failed: ") + e.what(),
            "Reduce batch size or check GPU memory"));
        metrics_.recordError("dispatch_failed");
        metrics_.recordKernelLaunchFailure();
        return {};
    }

    // CPU top-k selection from the distance matrix
    // Clamp k to available vectors to prevent out-of-bounds indexing
    const size_t effectiveK = std::min(k, numVectors);
    std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
    for (size_t q = 0; q < numQueries; ++q) {
        const float* row = distances.data() + q * numVectors;
        std::vector<std::pair<float, uint32_t>> row_pairs(numVectors);
        for (size_t v = 0; v < numVectors; ++v)
            row_pairs[v] = {row[v], static_cast<uint32_t>(v)};

        std::partial_sort(row_pairs.begin(),
                          row_pairs.begin() + static_cast<std::ptrdiff_t>(effectiveK),
                          row_pairs.end());

        results[q].resize(effectiveK);
        for (size_t i = 0; i < effectiveK; ++i)
            results[q][i] = {row_pairs[i].second, row_pairs[i].first};
    }
    clearError();
    return results;
#else
    return {};
#endif
}

// ============================================================================
// OpenGL Vector Backend Stub
// ============================================================================

OpenGLVectorBackend::~OpenGLVectorBackend() {
    shutdown();
}

bool OpenGLVectorBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_OPENGL
    // Check if OpenGL compute shaders are available (4.3+)
    return false; // Stub: not implemented yet
#else
    return false;
#endif
}

BackendCapabilities OpenGLVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_OPENGL
    caps.supportsVectorOps = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync = false;  // OpenGL compute is typically synchronous
    caps.deviceName = "OpenGL Compute (Stub)";
#endif
    return caps;
}

bool OpenGLVectorBackend::initialize() {
#ifdef THEMIS_ENABLE_OPENGL
    // Initialize OpenGL context
    initialized_ = false; // Stub
    return initialized_;
#else
    return false;
#endif
}

void OpenGLVectorBackend::shutdown() {
#ifdef THEMIS_ENABLE_OPENGL
    if (initialized_) {
        // Cleanup OpenGL resources
        initialized_ = false;
    }
#endif
}

std::vector<float> OpenGLVectorBackend::computeDistances(
    const float* /*queries*/,
    size_t /*numQueries*/,
    size_t /*dim*/,
    const float* /*vectors*/,
    size_t /*numVectors*/,
    bool /*useL2*/
) {
    return {}; // Stub
}

std::vector<std::vector<std::pair<uint32_t, float>>> OpenGLVectorBackend::batchKnnSearch(
    const float* /*queries*/,
    size_t /*numQueries*/,
    size_t /*dim*/,
    const float* /*vectors*/,
    size_t /*numVectors*/,
    size_t /*k*/,
    bool /*useL2*/
) {
    return {}; // Stub
}

// ============================================================================
// VulkanVectorBackend::populateANNDispatch
//
// Provides the frozen ANN kernel dispatch table for the Vulkan backend.
// All four slots (L2, cosine, inner product, top-K) are populated when
// compiled with THEMIS_ENABLE_VULKAN so the BackendRegistry can identify
// Vulkan as supporting all three distance metrics.
//
// The dispatch functions operate on host memory (the opaque_stream parameter
// is ignored).  GPU acceleration is provided through computeDistances() and
// batchKnnSearch() which manage Vulkan device memory internally.
// ============================================================================

#ifdef THEMIS_ENABLE_VULKAN

namespace {

static int vulkan_ann_l2_dispatch(
    const float* queries, const float* vectors, float* distances,
    int numQueries, int numVectors, int dim, void* /*stream*/)
{
    for (int q = 0; q < numQueries; ++q) {
        for (int v = 0; v < numVectors; ++v) {
            float sum = 0.f;
            for (int d = 0; d < dim; ++d) {
                float diff = queries[q * dim + d] - vectors[v * dim + d];
                sum += diff * diff;
            }
            distances[q * numVectors + v] = sum;
        }
    }
    return 0;
}

static int vulkan_ann_cosine_dispatch(
    const float* queries, const float* vectors, float* distances,
    int numQueries, int numVectors, int dim, void* /*stream*/)
{
    constexpr float kEps = 1e-10f;
    for (int q = 0; q < numQueries; ++q) {
        for (int v = 0; v < numVectors; ++v) {
            float dot = 0.f, nq = 0.f, nv = 0.f;
            for (int d = 0; d < dim; ++d) {
                float qv = queries[q * dim + d];
                float vv = vectors[v * dim + d];
                dot += qv * vv;
                nq  += qv * qv;
                nv  += vv * vv;
            }
            const float denom = std::sqrt(nq) * std::sqrt(nv);
            distances[q * numVectors + v] = (denom > kEps) ? 1.f - dot / denom : 1.f;
        }
    }
    return 0;
}

static int vulkan_ann_inner_product_dispatch(
    const float* queries, const float* vectors, float* distances,
    int numQueries, int numVectors, int dim, void* /*stream*/)
{
    for (int q = 0; q < numQueries; ++q) {
        for (int v = 0; v < numVectors; ++v) {
            float dot = 0.f;
            for (int d = 0; d < dim; ++d) {
                dot += queries[q * dim + d] * vectors[v * dim + d];
            }
            distances[q * numVectors + v] = -dot;
        }
    }
    return 0;
}

static int vulkan_ann_topk_dispatch(
    const float* distances, uint32_t* topk_indices, float* topk_dists,
    int numQueries, int numVectors, int topK, void* /*stream*/)
{
    using Pair = std::pair<float, uint32_t>;
    for (int q = 0; q < numQueries; ++q) {
        const float* row = distances + q * numVectors;
        std::priority_queue<Pair> heap;
        for (int v = 0; v < numVectors; ++v) {
            heap.emplace(row[v], static_cast<uint32_t>(v));
            if (static_cast<int>(heap.size()) > topK) heap.pop();
        }
        int slot = static_cast<int>(heap.size()) - 1;
        while (!heap.empty()) {
            topk_indices[q * topK + slot] = heap.top().second;
            topk_dists  [q * topK + slot] = heap.top().first;
            heap.pop();
            --slot;
        }
    }
    return 0;
}

// ---- Geospatial dispatch helpers (used by VulkanGeoBackend) ----

inline double vulkan_haversine_km(double lat1, double lon1,
                                   double lat2, double lon2) noexcept {
    constexpr double R   = 6371.0;
    constexpr double kPi = 3.141592653589793238462643383279502884;
    lat1 *= kPi / 180.0;
    lon1 *= kPi / 180.0;
    lat2 *= kPi / 180.0;
    lon2 *= kPi / 180.0;
    const double dlat = lat2 - lat1;
    const double dlon = lon2 - lon1;
    const double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
                     std::cos(lat1) * std::cos(lat2) *
                     std::sin(dlon / 2) * std::sin(dlon / 2);
    return R * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

static int vulkan_geo_distance(
    const double* lats1, const double* lons1,
    const double* lats2, const double* lons2,
    float* out_distances, int count,
    GeoDistanceFormula /*formula*/,
    void* /*stream*/)
{
    for (int i = 0; i < count; ++i) {
        out_distances[i] = static_cast<float>(
            vulkan_haversine_km(lats1[i], lons1[i], lats2[i], lons2[i]));
    }
    return 0;
}

static int vulkan_geo_containment(
    const double* point_lats, const double* point_lons, int numPoints,
    const double* polygon_coords, int numVertices,
    uint8_t* results, void* /*stream*/)
{
    for (int p = 0; p < numPoints; ++p) {
        const double testLat = point_lats[p];
        const double testLon = point_lons[p];
        bool inside = false;
        int  j = numVertices - 1;
        for (int i = 0; i < numVertices; ++i) {
            const double lat_i = polygon_coords[i * 2];
            const double lon_i = polygon_coords[i * 2 + 1];
            const double lat_j = polygon_coords[j * 2];
            const double lon_j = polygon_coords[j * 2 + 1];
            if (((lon_i > testLon) != (lon_j > testLon)) &&
                (testLat < (lat_j - lat_i) * (testLon - lon_i) / (lon_j - lon_i) + lat_i)) {
                inside = !inside;
            }
            j = i;
        }
        results[p] = inside ? 1u : 0u;
    }
    return 0;
}

} // anonymous namespace

#endif // THEMIS_ENABLE_VULKAN

ANNKernelDispatch VulkanVectorBackend::populateANNDispatch() const {
#ifdef THEMIS_ENABLE_VULKAN
    ANNKernelDispatch d;
    d.launchL2Distance   = vulkan_ann_l2_dispatch;
    d.launchCosine       = vulkan_ann_cosine_dispatch;
    d.launchInnerProduct = vulkan_ann_inner_product_dispatch;
    d.launchTopK         = vulkan_ann_topk_dispatch;
    return d;
#else
    return {}; // Vulkan not compiled — all null; BackendRegistry falls back to CPU table
#endif
}

// ============================================================================
// VulkanGeoBackend — geospatial compute backend
// ============================================================================

VulkanGeoBackend::VulkanGeoBackend() = default;

VulkanGeoBackend::~VulkanGeoBackend() {
    shutdown();
}

bool VulkanGeoBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_VULKAN
    // Reuse the same Vulkan instance probe as VulkanVectorBackend
    VkApplicationInfo appInfo{};
    appInfo.sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    VkInstance probe = VK_NULL_HANDLE;
    if (vkCreateInstance(&ci, nullptr, &probe) == VK_SUCCESS) {
        vkDestroyInstance(probe, nullptr);
        return true;
    }
    return false;
#else
    return false;
#endif
}

BackendCapabilities VulkanGeoBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_VULKAN
    caps.supportsGeoOps          = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;
    caps.deviceName              = initialized_ ? "Vulkan Geo" : "Vulkan Geo (not initialized)";
    caps.computeUnits            = 1;
#endif
    return caps;
}

bool VulkanGeoBackend::initialize() {
#ifdef THEMIS_ENABLE_VULKAN
    if (!isAvailable()) {
        // Vulkan ICD not available; all batchDistances / batchPointInPolygon
        // operations are implemented as CPU fallbacks so we can still
        // initialize successfully and serve requests.
        std::cerr << "[VulkanGeo] Vulkan ICD not available; running in CPU fallback mode"
                  << std::endl;
    }
    initialized_ = true;
    clearError();
    return true;
#else
    return false;
#endif
}

void VulkanGeoBackend::shutdown() {
    initialized_ = false;
}

std::vector<float> VulkanGeoBackend::batchDistances(
    const double* latitudes1,
    const double* longitudes1,
    const double* latitudes2,
    const double* longitudes2,
    size_t count,
    bool /*useHaversine*/
) {
#ifdef THEMIS_ENABLE_VULKAN
    if (!initialized_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "VulkanGeo",
            "VulkanGeo backend not initialized",
            "Call initialize() before using the backend"));
        return {};
    }
    if (latitudes1 == nullptr || longitudes1 == nullptr ||
        latitudes2 == nullptr || longitudes2 == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "VulkanGeo", AccelerationErrorCode::InvalidInputShape,
            "coordinate pointers must be non-null"));
        return {};
    }
    if (count == 0) { clearError(); return {}; }

    std::vector<float> out(count);
    for (size_t i = 0; i < count; ++i) {
        out[i] = static_cast<float>(
            vulkan_haversine_km(latitudes1[i], longitudes1[i],
                                latitudes2[i], longitudes2[i]));
    }
    return out;
#else
    return {};
#endif
}

std::vector<bool> VulkanGeoBackend::batchPointInPolygon(
    const double* pointLats,
    const double* pointLons,
    size_t numPoints,
    const double* polygonCoords,
    size_t numPolygonVertices
) {
#ifdef THEMIS_ENABLE_VULKAN
    if (!initialized_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "VulkanGeo",
            "VulkanGeo backend not initialized",
            "Call initialize() before using the backend"));
        return {};
    }
    if (pointLats == nullptr || pointLons == nullptr || polygonCoords == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "VulkanGeo", AccelerationErrorCode::InvalidInputShape,
            "point and polygon coordinate pointers must be non-null"));
        return {};
    }
    if (numPoints == 0) { clearError(); return {}; }
    if (numPolygonVertices < 3) {
        setError(ErrorContextHelpers::createValidationError(
            "VulkanGeo", AccelerationErrorCode::InvalidInputShape,
            "polygon must have at least 3 vertices"));
        return {};
    }

    std::vector<bool> out(numPoints, false);
    const int nv = static_cast<int>(numPolygonVertices);
    for (size_t p = 0; p < numPoints; ++p) {
        const double testLat = pointLats[p];
        const double testLon = pointLons[p];
        bool inside = false;
        int  j = nv - 1;
        for (int i = 0; i < nv; ++i) {
            const double lat_i = polygonCoords[i * 2];
            const double lon_i = polygonCoords[i * 2 + 1];
            const double lat_j = polygonCoords[j * 2];
            const double lon_j = polygonCoords[j * 2 + 1];
            if (((lon_i > testLon) != (lon_j > testLon)) &&
                (testLat < (lat_j - lat_i) * (testLon - lon_i) / (lon_j - lon_i) + lat_i)) {
                inside = !inside;
            }
            j = i;
        }
        out[p] = inside;
    }
    return out;
#else
    return {};
#endif
}

GeoKernelDispatch VulkanGeoBackend::populateGeoDispatch() const {
#ifdef THEMIS_ENABLE_VULKAN
    GeoKernelDispatch d;
    d.launchDistance    = vulkan_geo_distance;
    d.launchContainment = vulkan_geo_containment;
    return d;
#else
    return {}; // Vulkan not compiled — all null; BackendRegistry falls back to CPU table
#endif
}

} // namespace acceleration
} // namespace themis
