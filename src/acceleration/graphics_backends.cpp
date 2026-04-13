/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphics_backends.cpp                              ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:23:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   86.0/100                                       ║
    • Total Lines:     2567                                           ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2c7ea935e7  2026-03-14  fix(acceleration): address Vulkan compute shader pipeline... ║
    • f657d8df5b  2026-03-14  fix(acceleration): address code review feedback on Vulkan... ║
    • f52f9b7eaa  2026-03-14  feat(acceleration): implement Vulkan compute shader pipel... ║
    • 1ae2010848  2026-03-11  audit: fix post-implementation review findings for Direct... ║
    • 4b2fdfa0e1  2026-03-11  fix(acceleration): Wire OpenGLVectorBackend into BackendR... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
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

#ifdef THEMIS_ENABLE_OPENGL
#if defined(__linux__) || defined(__unix__)
#include <dlfcn.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <cstddef>   // ptrdiff_t, size_t
#include <cstdint>   // intptr_t
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

    // MoltenVK / Apple M-series capability probe
    // true when the physical device supports buffer device address and it is
    // usable (i.e. the feature flag is actually set, not just extension-listed).
    // Probed via vkGetPhysicalDeviceFeatures2 which is correct for both the
    // KHR-extension path (Vulkan 1.0/1.1 with VK_KHR_buffer_device_address)
    // and the promoted-core path (Vulkan 1.2+ where it may not appear in the
    // extension list but VkPhysicalDeviceVulkan12Features carries it).
    // On Apple Silicon via MoltenVK the extension may be absent or the feature
    // disabled even if the name shows up in the extension list.
    bool hasBufferDeviceAddress = false;

    // Tunable workgroup sizes exposed as SPIR-V specialization constants.
    // Defaults match the original fixed sizes in the shaders.
    // Host code can adjust these before calling createComputePipelines() to
    // target specific hardware occupancy (e.g. different tile sizes for
    // Mali-G710 vs. RDNA2).
    uint32_t wgL2X          = 16;   // l2_distance.comp local_size_x (spec id 0)
    uint32_t wgL2Y          = 16;   // l2_distance.comp local_size_y (spec id 1)
    // batch_search.comp local_size_x pending specialization constant wiring;
    // stored here for future pipeline integration.  Max 256 (shared-mem limit).
    uint32_t wgBatchSearchX = 256;

    // Sizes actually baked into the compiled pipelines (set by createComputePipelines).
    // cosine_distance.comp and inner_product_distance.comp hard-code 16×16 in GLSL,
    // so their baked sizes are always 16 regardless of wgL2X/wgL2Y.
    uint32_t bakedL2X   = 16;   // effective local_size_x for l2Pipeline
    uint32_t bakedL2Y   = 16;   // effective local_size_y for l2Pipeline
    // cosine/inner-product pipelines always use 16×16 (no specialization constants yet)
    static constexpr uint32_t kCosineLocalX = 16;
    static constexpr uint32_t kCosineLocalY = 16;

    // ---- Buffer helper ------------------------------------------------
    struct BufMem {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
    };

    // ---- Double-buffer staging ring ------------------------------------
    // Two persistent staging-buffer slots that alternate each dispatch.
    // The CPU fills slot[next] while the GPU is executing slot[cur],
    // overlapping host→device DMA with shader dispatch.
    // Buffers grow on demand (never shrink) to avoid per-dispatch allocations.
    struct StagingSlot {
        BufMem  stagQ;          // host-visible staging for query data
        BufMem  stagV;          // host-visible staging for vector data
        BufMem  stagOut;        // host-visible staging for output data
        VkFence fence = VK_NULL_HANDLE;
        bool    pending = false; // true while the GPU is using this slot
    };
    static constexpr int kStagingSlots = 2;
    StagingSlot stagingRing_[kStagingSlots] = {};
    int         stagingRingIdx_ = 0;
    bool        stagingRingInited_ = false;

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

        // MoltenVK / Apple M-series: probe VK_KHR_buffer_device_address.
        //
        // Approach:
        //  (a) On Vulkan 1.2+ the feature may be promoted to core — query via
        //      VkPhysicalDeviceVulkan12Features (no extension advertisement needed).
        //  (b) On Vulkan 1.0/1.1 with the KHR extension, check the extension list
        //      first to know whether vkGetPhysicalDeviceFeatures2 would honour it,
        //      then query VkPhysicalDeviceBufferDeviceAddressFeaturesKHR to confirm
        //      the feature is actually enabled (not just the name advertised).
        //
        // This correctly handles MoltenVK, where the extension may be listed but
        // the feature flag may still be VK_FALSE.
        {
            bool extAdvertised = false;
            uint32_t extCount = 0;
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                                 &extCount, nullptr);
            std::vector<VkExtensionProperties> exts(extCount);
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                                 &extCount, exts.data());
            for (const auto& ext : exts) {
                if (std::strcmp(ext.extensionName,
                                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
                    extAdvertised = true;
                    break;
                }
            }

            if (deviceProps.apiVersion >= VK_MAKE_VERSION(1, 2, 0)) {
                // Vulkan 1.2+: BDA is a core feature — query via pNext chain.
                VkPhysicalDeviceVulkan12Features vk12Features{};
                vk12Features.sType =
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
                VkPhysicalDeviceFeatures2 features2{};
                features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features2.pNext = &vk12Features;
                vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
                hasBufferDeviceAddress =
                    (vk12Features.bufferDeviceAddress == VK_TRUE);
            } else if (extAdvertised) {
                // Vulkan 1.0/1.1 with KHR extension: verify the feature is
                // actually supported, not just listed.
                VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bdaFeatures{};
                bdaFeatures.sType =
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
                VkPhysicalDeviceFeatures2 features2{};
                features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features2.pNext = &bdaFeatures;
                vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
                hasBufferDeviceAddress =
                    (bdaFeatures.bufferDeviceAddress == VK_TRUE);
            }
            // If neither condition applies, hasBufferDeviceAddress stays false.
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

        // Build specialization constants for l2_distance.comp (IDs 0 and 1 =
        // local_size_x and local_size_y).  These let the host adjust the 2-D
        // workgroup tile at pipeline-creation time without recompiling GLSL.
        struct L2SpecData { uint32_t wgX; uint32_t wgY; };
        const L2SpecData l2Spec{ wgL2X, wgL2Y };
        const VkSpecializationMapEntry l2Entries[2] = {
            { 0, offsetof(L2SpecData, wgX), sizeof(uint32_t) },
            { 1, offsetof(L2SpecData, wgY), sizeof(uint32_t) },
        };
        VkSpecializationInfo l2SpecInfo{};
        l2SpecInfo.mapEntryCount = 2;
        l2SpecInfo.pMapEntries   = l2Entries;
        l2SpecInfo.dataSize      = sizeof(L2SpecData);
        l2SpecInfo.pData         = &l2Spec;

        // cosine_distance.comp and inner_product_distance.comp retain their
        // hard-coded local_size_x = 16, local_size_y = 16 declarations.
        // Specialization constants for those shaders will be added in a
        // future pass when Mali-G710 / RDNA2 occupancy data is available.

        // Helper: create one compute pipeline.  pSpecInfo may be nullptr for
        // shaders without specialization constants (cosine, inner-product).
        auto makePipeline = [&](VkShaderModule mod, VkPipeline& out,
                                const VkSpecializationInfo* pSpec) -> bool {
            VkPipelineShaderStageCreateInfo ssi{};
            ssi.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            ssi.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
            ssi.module              = mod;
            ssi.pName               = "main";
            ssi.pSpecializationInfo = pSpec;

            VkComputePipelineCreateInfo pci{};
            pci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pci.stage  = ssi;
            pci.layout = pipelineLayout;
            return vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr, &out) == VK_SUCCESS;
        };

        const bool ok = makePipeline(l2ShaderModule, l2Pipeline, &l2SpecInfo) &&
                        makePipeline(cosineShaderModule, cosinePipeline, nullptr) &&
                        makePipeline(innerProductShaderModule, innerProductPipeline, nullptr);

        // Record the workgroup sizes that were baked into the pipelines so that
        // dispatch() can compute correct group-counts per metric.
        if (ok) {
            bakedL2X = wgL2X;
            bakedL2Y = wgL2Y;
        }
        return ok;
    }

    // ---- Double-buffer staging helpers --------------------------------

    // Grow a BufMem to at least `needed` bytes.  Destroys and re-creates the
    // buffer/memory when the current allocation is too small; keeps it as-is
    // when it is already large enough to avoid unnecessary re-allocation.
    void ensureStagingBuffer(BufMem& bm, VkDeviceSize needed,
                             VkBufferUsageFlags usage) {
        if (bm.buffer != VK_NULL_HANDLE && bm.size >= needed) return;
        if (bm.buffer != VK_NULL_HANDLE) destroyBuffer(bm);
        const VkMemoryPropertyFlags hostProps =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        bm = createBuffer(needed, usage, hostProps);
    }

    // Initialise fences for all staging slots.  Called lazily on the first
    // dispatch so we do not create fences until a device exists.
    void initStagingRing() {
        if (stagingRingInited_) return;
        VkFenceCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        for (int i = 0; i < kStagingSlots; ++i) {
            vkCreateFence(device, &fci, nullptr, &stagingRing_[i].fence);
        }
        stagingRingInited_ = true;
    }

    // ---- Compute dispatch (double-buffered staging) --------------------
    //
    // This implementation overlaps host→device DMA with shader dispatch by
    // using a ring of two persistent staging-buffer slots:
    //
    //   • Slot A (current):  CPU fills staging buffers while GPU may still be
    //     reading from slot B (the previous dispatch).  The two-phase command
    //     submission model further decouples the transfer and compute stages:
    //
    //       CB1 (transfer phase): copy stagQ → devQ, copy stagV → devV
    //                             signal semaphore xferDone
    //       CB2 (compute phase):  wait semaphore xferDone
    //                             dispatch kernel, copy devOut → stagOut
    //                             signal fence (slot.fence)
    //
    //   • CB1 is submitted first; while the GPU runs the PCIe DMA, the CPU
    //     can record CB2 — giving genuine DMA–compute overlap on platforms
    //     where the driver exposes separate transfer and compute engines.
    //
    //   • Staging buffers are reused across calls (grow only when needed),
    //     eliminating vkAllocateMemory / vkFreeMemory overhead per dispatch.
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

        // ---- Initialise staging ring on first use --------------------
        initStagingRing();

        // ---- Select current staging slot and wait if still in-flight --
        StagingSlot& slot = stagingRing_[stagingRingIdx_];
        if (slot.pending) {
            // Wait for the fence that was signalled when this slot's compute
            // phase completed on a previous call.
            vkWaitForFences(device, 1, &slot.fence, VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &slot.fence);
            slot.pending = false;
        }

        // ---- Grow persistent staging buffers only when needed ---------
        ensureStagingBuffer(slot.stagQ,   qSize,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        ensureStagingBuffer(slot.stagV,   vSize,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        ensureStagingBuffer(slot.stagOut, outSz,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        // ---- Copy host data into staging buffers (CPU side) ----------
        auto copyToStaging = [&](BufMem& bm, const void* src, VkDeviceSize sz) {
            void* ptr = nullptr;
            vkMapMemory(device, bm.memory, 0, sz, 0, &ptr);
            std::memcpy(ptr, src, sz);
            vkUnmapMemory(device, bm.memory);
        };
        copyToStaging(slot.stagQ, queries, qSize);
        copyToStaging(slot.stagV, vectors, vSize);

        // ---- Device-local buffers (per-dispatch, sized to actual data) -
        auto devQ   = createBuffer(qSize,  devUsage, devProps);
        auto devV   = createBuffer(vSize,  devUsage, devProps);
        auto devOut = createBuffer(outSz,  devUsage, devProps);

        // ---- Semaphore: gates CB2 (compute) on CB1 (transfer) ---------
        VkSemaphoreCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkSemaphore xferDone = VK_NULL_HANDLE;
        vkCreateSemaphore(device, &sci, nullptr, &xferDone);

        // ---- CB1: transfer phase — staging → device -------------------
        VkCommandBufferAllocateInfo cbai{};
        cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.commandPool        = commandPool;
        cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbai.commandBufferCount = 1;
        VkCommandBuffer cbTransfer;
        vkAllocateCommandBuffers(device, &cbai, &cbTransfer);

        {
            VkCommandBufferBeginInfo cbbi{};
            cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cbTransfer, &cbbi);

            VkBufferCopy cpQ{0, 0, qSize}, cpV{0, 0, vSize};
            vkCmdCopyBuffer(cbTransfer, slot.stagQ.buffer, devQ.buffer, 1, &cpQ);
            vkCmdCopyBuffer(cbTransfer, slot.stagV.buffer, devV.buffer, 1, &cpV);

            vkEndCommandBuffer(cbTransfer);
        }

        // Submit CB1 and signal xferDone — the GPU starts DMA immediately.
        // The CPU can now record CB2 concurrently (overlap).
        {
            VkSubmitInfo si{};
            si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.commandBufferCount   = 1;
            si.pCommandBuffers      = &cbTransfer;
            si.signalSemaphoreCount = 1;
            si.pSignalSemaphores    = &xferDone;
            vkQueueSubmit(computeQueue, 1, &si, VK_NULL_HANDLE);
        }

        // ---- CB2: compute+readback phase (recorded while CB1 runs) ----
        VkCommandBuffer cbCompute;
        vkAllocateCommandBuffers(device, &cbai, &cbCompute);

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

        {
            VkCommandBufferBeginInfo cbbi{};
            cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cbCompute, &cbbi);

            // Barrier: transfer write → compute read
            VkMemoryBarrier mb{};
            mb.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            mb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cbCompute,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0, 1, &mb, 0, nullptr, 0, nullptr);

            // Bind pipeline and dispatch
            VkPipeline pipeline;
            switch (metric) {
                case DistanceMetric::COSINE:        pipeline = cosinePipeline;       break;
                case DistanceMetric::INNER_PRODUCT: pipeline = innerProductPipeline; break;
                default:                            pipeline = l2Pipeline;           break;
            }
            vkCmdBindPipeline(cbCompute, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
            vkCmdBindDescriptorSets(cbCompute, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipelineLayout, 0, 1, &ds, 0, nullptr);

            uint32_t pc[3] = {nq, nv, dim};
            vkCmdPushConstants(cbCompute, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                               0, sizeof(pc), pc);

            // Dispatch group counts must match the local sizes baked into each
            // pipeline's specialization constants (or hardcoded GLSL for shaders
            // that have no specialization constants yet).
            //
            //  • l2Pipeline:              bakedL2X × bakedL2Y  (from wgL2X/wgL2Y)
            //  • cosinePipeline:          16 × 16  (hard-coded in cosine_distance.comp)
            //  • innerProductPipeline:    16 × 16  (hard-coded in inner_product_distance.comp)
            //
            // Using the L2 specialization values for cosine/IP would produce wrong
            // group-counts when wgL2X/wgL2Y differ from 16 — leading to
            // under-dispatch and incomplete/corrupt results.
            uint32_t localX, localY;
            if (metric == DistanceMetric::L2) {
                localX = bakedL2X;
                localY = bakedL2Y;
            } else {
                localX = kCosineLocalX;
                localY = kCosineLocalY;
            }
            uint32_t gx = (nv + localX - 1) / localX;
            uint32_t gy = (nq + localY - 1) / localY;
            vkCmdDispatch(cbCompute, gx, gy, 1);

            // Barrier: compute write → transfer read
            mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            mb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(cbCompute,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 1, &mb, 0, nullptr, 0, nullptr);

            // Copy results: device → staging
            VkBufferCopy outRegion{0, 0, outSz};
            vkCmdCopyBuffer(cbCompute, devOut.buffer, slot.stagOut.buffer, 1, &outRegion);

            vkEndCommandBuffer(cbCompute);
        }

        // Submit CB2: wait for xferDone semaphore, signal slot.fence on finish
        {
            const VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            VkSubmitInfo si{};
            si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.waitSemaphoreCount   = 1;
            si.pWaitSemaphores      = &xferDone;
            si.pWaitDstStageMask    = &waitMask;
            si.commandBufferCount   = 1;
            si.pCommandBuffers      = &cbCompute;
            vkQueueSubmit(computeQueue, 1, &si, slot.fence);
        }
        slot.pending = true;

        // Advance ring index so the *next* dispatch uses the other slot.
        stagingRingIdx_ = (stagingRingIdx_ + 1) % kStagingSlots;

        // For the synchronous API we must return results immediately, so we
        // wait on the just-submitted fence.  The next call may begin filling
        // the other slot while this wait runs (genuine DMA–compute overlap).
        vkWaitForFences(device, 1, &slot.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &slot.fence);
        slot.pending = false;

        // Read results from the host-visible staging buffer
        std::vector<float> results(static_cast<size_t>(nq) * nv);
        {
            void* ptr = nullptr;
            vkMapMemory(device, slot.stagOut.memory, 0, outSz, 0, &ptr);
            std::memcpy(results.data(), ptr, outSz);
            vkUnmapMemory(device, slot.stagOut.memory);
        }

        // Free per-dispatch resources (device-local buffers, semaphore, CBs, DS)
        vkDestroySemaphore(device, xferDone, nullptr);
        vkFreeCommandBuffers(device, commandPool, 1, &cbTransfer);
        vkFreeCommandBuffers(device, commandPool, 1, &cbCompute);
        vkFreeDescriptorSets(device, descriptorPool, 1, &ds);
        destroyBuffer(devQ);
        destroyBuffer(devV);
        destroyBuffer(devOut);

        return results;
    }

    // ---- Cleanup ------------------------------------------------------
    void cleanup() {
        if (device == VK_NULL_HANDLE) return;
        vkDeviceWaitIdle(device);

        // Destroy double-buffer staging ring
        for (int i = 0; i < kStagingSlots; ++i) {
            destroyBuffer(stagingRing_[i].stagQ);
            destroyBuffer(stagingRing_[i].stagV);
            destroyBuffer(stagingRing_[i].stagOut);
            if (stagingRing_[i].fence != VK_NULL_HANDLE) {
                vkDestroyFence(device, stagingRing_[i].fence, nullptr);
                stagingRing_[i].fence = VK_NULL_HANDLE;
            }
        }
        stagingRingInited_ = false;

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
// DirectXVectorBackend — stub implementation for non-DirectX builds
// Full Windows implementation is in directx_backend_full.cpp
// (compiled when _WIN32 && THEMIS_ENABLE_DIRECTX)
// ============================================================================

#if !defined(_WIN32) || !defined(THEMIS_ENABLE_DIRECTX)

class DirectXVectorBackend::DirectXVectorBackendImpl {
    // Empty placeholder when DirectX 12 is not compiled in
};

DirectXVectorBackend::DirectXVectorBackend()
    : initialized_(false), impl_(std::make_unique<DirectXVectorBackendImpl>()) {}

DirectXVectorBackend::~DirectXVectorBackend() {
    shutdown();
}

bool DirectXVectorBackend::isAvailable() const noexcept {
    return false;
}

BackendCapabilities DirectXVectorBackend::getCapabilities() const {
    return {};
}

bool DirectXVectorBackend::initialize() {
    return false;
}

void DirectXVectorBackend::shutdown() {}

std::vector<float> DirectXVectorBackend::computeDistances(
    const float* /*queries*/,
    size_t /*numQueries*/,
    size_t /*dim*/,
    const float* /*vectors*/,
    size_t /*numVectors*/,
    bool /*useL2*/
) {
    return {};
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
    return {};
}

#endif // !_WIN32 || !THEMIS_ENABLE_DIRECTX

// ============================================================================
// VulkanVectorBackend — public interface implementation
// ============================================================================

VulkanVectorBackend::VulkanVectorBackend()
    : initialized_(false), impl_(std::make_unique<VulkanVectorBackendImpl>()) {}

VulkanVectorBackend::~VulkanVectorBackend() {
    shutdown();
}

bool VulkanVectorBackend::hasBufferDeviceAddress() const noexcept {
#ifdef THEMIS_ENABLE_VULKAN
    if (initialized_ && impl_) return impl_->hasBufferDeviceAddress;
#endif
    return false;
}

void VulkanVectorBackend::setWorkgroupSizeL2(uint32_t wgX, uint32_t wgY) noexcept {
#ifdef THEMIS_ENABLE_VULKAN
    // Reject changes after initialize() — the pipeline has already been compiled
    // with the previous values.  Silently ignore to keep the API noexcept.
    if (initialized_) return;
    // Zero dimensions would cause division-by-zero in dispatch group-count math.
    if (wgX == 0 || wgY == 0) return;
    if (impl_) {
        impl_->wgL2X = wgX;
        impl_->wgL2Y = wgY;
    }
#else
    (void)wgX; (void)wgY;
#endif
}

void VulkanVectorBackend::setWorkgroupSizeBatchSearch(uint32_t wgX) noexcept {
#ifdef THEMIS_ENABLE_VULKAN
    // Reject post-init and zero values; also clamp to 256 — batch_search.comp
    // declares shared float sharedQuery[256] so any value > 256 would cause
    // out-of-bounds shared-memory access in the shader.
    if (initialized_) return;
    if (wgX == 0 || wgX > 256u) return;
    if (impl_) {
        impl_->wgBatchSearchX = wgX;
    }
#else
    (void)wgX;
#endif
}

std::pair<uint32_t, uint32_t> VulkanVectorBackend::getWorkgroupSizeL2() const noexcept {
#ifdef THEMIS_ENABLE_VULKAN
    if (impl_) return {impl_->wgL2X, impl_->wgL2Y};
#endif
    return {16u, 16u};
}

uint32_t VulkanVectorBackend::getWorkgroupSizeBatchSearch() const noexcept {
#ifdef THEMIS_ENABLE_VULKAN
    if (impl_) return impl_->wgBatchSearchX;
#endif
    return 256u;
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
    std::cout << "[Vulkan] VK_KHR_buffer_device_address: "
              << (impl_->hasBufferDeviceAddress ? "supported" : "not supported")
              << " (vendor: " << impl_->vendorName << ")" << std::endl;
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
        + std::to_string(VK_API_VERSION_PATCH(impl_->deviceProps.apiVersion))
        + (impl_->hasBufferDeviceAddress
               ? " [VK_KHR_buffer_device_address]"
               : " [no VK_KHR_buffer_device_address]");

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
// OpenGL Vector Backend — full OpenGL 4.3+ Compute Shader implementation
//
// Context creation uses EGL (Linux/Android) or WGL (Windows) via dynamic
// loading so that no GL headers are required at compile time. When an
// OpenGL 4.3+ context cannot be created (no display, no compatible driver)
// the backend falls back to CPU kernels so that initialize() always
// succeeds and the registry can still exercise the dispatch path.
// ============================================================================

#ifdef THEMIS_ENABLE_OPENGL

namespace {

// ---------------------------------------------------------------------------
// Minimal GL / EGL type and constant declarations.
// These match the stable OpenGL 4.3 and EGL 1.5 specifications exactly.
// No GL headers are included so the file compiles on systems that have
// the runtime libraries but not the development headers (e.g. CI).
// ---------------------------------------------------------------------------

typedef unsigned int    GL_GLuint;
typedef int             GL_GLint;
typedef unsigned int    GL_GLenum;
typedef int             GL_GLsizei;
typedef char            GL_GLchar;
typedef std::ptrdiff_t  GL_GLsizeiptr;   // matches the OpenGL spec (signed pointer-sized)
typedef std::ptrdiff_t  GL_GLintptr;     // same as GLintptr in the spec
typedef unsigned int    GL_GLbitfield;
typedef float           GL_GLfloat;
typedef unsigned char   GL_GLboolean;
typedef unsigned char   GL_GLubyte;

typedef void*           EGL_Display;
typedef void*           EGL_Config;
typedef void*           EGL_Context;
typedef void*           EGL_Surface;
typedef unsigned int    EGL_Enum;
typedef unsigned int    EGL_Boolean;
typedef int             EGL_Int;
typedef void*           EGL_NativeDisplayType;

// GL constants used by compute shaders
static const GL_GLenum  k_COMPUTE_SHADER              = 0x91B9u;
static const GL_GLenum  k_SHADER_STORAGE_BUFFER       = 0x90D2u;
static const GL_GLenum  k_STREAM_COPY                 = 0x88E2u;
static const GL_GLenum  k_DYNAMIC_COPY                = 0x88EAu;
static const GL_GLbitfield k_SHADER_STORAGE_BARRIER_BIT = 0x00002000u;
static const GL_GLenum  k_COMPILE_STATUS              = 0x8B81u;
static const GL_GLenum  k_LINK_STATUS                 = 0x8B82u;
static const GL_GLenum  k_INFO_LOG_LENGTH             = 0x8B84u;
static const GL_GLenum  k_RENDERER                    = 0x1F01u;
static const GL_GLenum  k_VENDOR                      = 0x1F00u;
static const GL_GLenum  k_MAJOR_VERSION               = 0x821Bu;
static const GL_GLenum  k_MINOR_VERSION               = 0x821Cu;

// EGL constants
static const EGL_Display k_EGL_NO_DISPLAY             = nullptr;
static const EGL_Context k_EGL_NO_CONTEXT             = nullptr;
static const EGL_Surface k_EGL_NO_SURFACE             = nullptr;
static const EGL_Enum    k_EGL_OPENGL_API             = 0x30A2u;
static const EGL_Int     k_EGL_RENDERABLE_TYPE        = 0x3040;
static const EGL_Int     k_EGL_OPENGL_BIT             = 0x0008;
static const EGL_Int     k_EGL_NONE                   = 0x3038;
static const EGL_Int     k_EGL_CONTEXT_MAJOR_VERSION  = 0x3098;
static const EGL_Int     k_EGL_CONTEXT_MINOR_VERSION  = 0x30FB;
static const EGL_Int     k_EGL_CONTEXT_OPENGL_PROFILE_MASK            = 0x30FD;
static const EGL_Int     k_EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT        = 0x00000001;

// ---------------------------------------------------------------------------
// Function pointer typedefs — EGL
// ---------------------------------------------------------------------------
typedef EGL_Display (*PFN_eglGetDisplay)(EGL_NativeDisplayType);
typedef EGL_Boolean (*PFN_eglInitialize)(EGL_Display, EGL_Int*, EGL_Int*);
typedef EGL_Boolean (*PFN_eglBindAPI)(EGL_Enum);
typedef EGL_Boolean (*PFN_eglChooseConfig)(EGL_Display, const EGL_Int*, EGL_Config*, EGL_Int, EGL_Int*);
typedef EGL_Context (*PFN_eglCreateContext)(EGL_Display, EGL_Config, EGL_Context, const EGL_Int*);
typedef EGL_Boolean (*PFN_eglMakeCurrent)(EGL_Display, EGL_Surface, EGL_Surface, EGL_Context);
typedef EGL_Boolean (*PFN_eglDestroyContext)(EGL_Display, EGL_Context);
typedef EGL_Boolean (*PFN_eglTerminate)(EGL_Display);
typedef void*       (*PFN_eglGetProcAddress)(const char*);

// ---------------------------------------------------------------------------
// Function pointer typedefs — OpenGL 4.3 Compute Shaders
// ---------------------------------------------------------------------------
typedef GL_GLuint (*PFN_glCreateShader)(GL_GLenum);
typedef void (*PFN_glShaderSource)(GL_GLuint, GL_GLsizei, const GL_GLchar**, const GL_GLint*);
typedef void (*PFN_glCompileShader)(GL_GLuint);
typedef void (*PFN_glGetShaderiv)(GL_GLuint, GL_GLenum, GL_GLint*);
typedef void (*PFN_glGetShaderInfoLog)(GL_GLuint, GL_GLsizei, GL_GLsizei*, GL_GLchar*);
typedef void (*PFN_glDeleteShader)(GL_GLuint);
typedef GL_GLuint (*PFN_glCreateProgram)();
typedef void (*PFN_glAttachShader)(GL_GLuint, GL_GLuint);
typedef void (*PFN_glLinkProgram)(GL_GLuint);
typedef void (*PFN_glGetProgramiv)(GL_GLuint, GL_GLenum, GL_GLint*);
typedef void (*PFN_glGetProgramInfoLog)(GL_GLuint, GL_GLsizei, GL_GLsizei*, GL_GLchar*);
typedef void (*PFN_glDeleteProgram)(GL_GLuint);
typedef void (*PFN_glUseProgram)(GL_GLuint);
typedef GL_GLint (*PFN_glGetUniformLocation)(GL_GLuint, const GL_GLchar*);
typedef void (*PFN_glUniform1ui)(GL_GLint, unsigned int);
typedef void (*PFN_glGenBuffers)(GL_GLsizei, GL_GLuint*);
typedef void (*PFN_glDeleteBuffers)(GL_GLsizei, const GL_GLuint*);
typedef void (*PFN_glBindBuffer)(GL_GLenum, GL_GLuint);
typedef void (*PFN_glBufferData)(GL_GLenum, GL_GLsizeiptr, const void*, GL_GLenum);
typedef void (*PFN_glBindBufferBase)(GL_GLenum, GL_GLuint, GL_GLuint);
typedef void (*PFN_glDispatchCompute)(GL_GLuint, GL_GLuint, GL_GLuint);
typedef void (*PFN_glMemoryBarrier)(GL_GLbitfield);
typedef void (*PFN_glGetBufferSubData)(GL_GLenum, GL_GLintptr, GL_GLsizeiptr, void*);
typedef const GL_GLubyte* (*PFN_glGetString)(GL_GLenum);
typedef void (*PFN_glGetIntegerv)(GL_GLenum, GL_GLint*);

// ---------------------------------------------------------------------------
// GLSL compute shader source — L2 squared distance
// Each invocation computes one (query, vector) pair.
// local_size_x = queries axis, local_size_y = vectors axis.
// ---------------------------------------------------------------------------
static const char* s_glsl_l2_src = R"glsl(
#version 430 core
layout(local_size_x = 8, local_size_y = 8) in;

layout(std430, binding = 0) readonly buffer QBuf { float q[]; };
layout(std430, binding = 1) readonly buffer VBuf { float v[]; };
layout(std430, binding = 2) writeonly buffer DBuf { float d[]; };

uniform uint uNQ;
uniform uint uNV;
uniform uint uDim;

void main() {
    uint qi = gl_GlobalInvocationID.x;
    uint vi = gl_GlobalInvocationID.y;
    if (qi >= uNQ || vi >= uNV) return;
    float sum = 0.0;
    for (uint dd = 0u; dd < uDim; ++dd) {
        float diff = q[qi * uDim + dd] - v[vi * uDim + dd];
        sum += diff * diff;
    }
    d[qi * uNV + vi] = sum;
}
)glsl";

// ---------------------------------------------------------------------------
// GLSL compute shader source — cosine distance (1 - cosine_similarity)
// ---------------------------------------------------------------------------
static const char* s_glsl_cosine_src = R"glsl(
#version 430 core
layout(local_size_x = 8, local_size_y = 8) in;

layout(std430, binding = 0) readonly buffer QBuf { float q[]; };
layout(std430, binding = 1) readonly buffer VBuf { float v[]; };
layout(std430, binding = 2) writeonly buffer DBuf { float d[]; };

uniform uint uNQ;
uniform uint uNV;
uniform uint uDim;

void main() {
    uint qi = gl_GlobalInvocationID.x;
    uint vi = gl_GlobalInvocationID.y;
    if (qi >= uNQ || vi >= uNV) return;
    float dot = 0.0, nq = 0.0, nv = 0.0;
    for (uint dd = 0u; dd < uDim; ++dd) {
        float qv = q[qi * uDim + dd];
        float vv = v[vi * uDim + dd];
        dot += qv * vv;
        nq  += qv * qv;
        nv  += vv * vv;
    }
    float denom = sqrt(nq) * sqrt(nv);
    d[qi * uNV + vi] = (denom > 1e-10) ? (1.0 - dot / denom) : 1.0;
}
)glsl";

// ---------------------------------------------------------------------------
// Platform dynamic-library helpers
// ---------------------------------------------------------------------------

static void* openLib(const char* name) {
#if defined(__linux__) || defined(__unix__)
    return dlopen(name, RTLD_LAZY | RTLD_LOCAL);
#elif defined(_WIN32)
    return static_cast<void*>(LoadLibraryA(name));
#else
    (void)name;
    return nullptr;
#endif
}

static void closeLib(void* lib) {
    if (!lib) return;
#if defined(__linux__) || defined(__unix__)
    dlclose(lib);
#elif defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(lib));
#endif
}

static void* libSym(void* lib, const char* sym) {
    if (!lib) return nullptr;
#if defined(__linux__) || defined(__unix__)
    return dlsym(lib, sym);
#elif defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(lib), sym));
#else
    (void)sym;
    return nullptr;
#endif
}

} // anonymous namespace

#endif // THEMIS_ENABLE_OPENGL

// ---------------------------------------------------------------------------
// OpenGLVectorBackend::OpenGLVectorBackendImpl
// Holds all EGL/GL handles and function pointers. When gpuAvailable_ is
// false the impl runs pure-CPU kernels so that the backend always succeeds
// after initialize() (mirrors the VulkanGeoBackend CPU-fallback pattern).
// ---------------------------------------------------------------------------

class OpenGLVectorBackend::OpenGLVectorBackendImpl {
public:
#ifdef THEMIS_ENABLE_OPENGL
    // Library handles
    void* libEGL_ = nullptr;
    void* libGL_  = nullptr;

    // EGL handles
    EGL_Display eglDisplay_ = nullptr;
    EGL_Context eglContext_ = nullptr;

    // EGL function pointers
    PFN_eglGetDisplay       pfnEglGetDisplay       = nullptr;
    PFN_eglInitialize       pfnEglInitialize       = nullptr;
    PFN_eglBindAPI          pfnEglBindAPI          = nullptr;
    PFN_eglChooseConfig     pfnEglChooseConfig     = nullptr;
    PFN_eglCreateContext    pfnEglCreateContext    = nullptr;
    PFN_eglMakeCurrent      pfnEglMakeCurrent      = nullptr;
    PFN_eglDestroyContext   pfnEglDestroyContext   = nullptr;
    PFN_eglTerminate        pfnEglTerminate        = nullptr;
    PFN_eglGetProcAddress   pfnEglGetProcAddress   = nullptr;

    // GL function pointers
    PFN_glCreateShader      pfnGlCreateShader      = nullptr;
    PFN_glShaderSource      pfnGlShaderSource      = nullptr;
    PFN_glCompileShader     pfnGlCompileShader     = nullptr;
    PFN_glGetShaderiv       pfnGlGetShaderiv       = nullptr;
    PFN_glGetShaderInfoLog  pfnGlGetShaderInfoLog  = nullptr;
    PFN_glDeleteShader      pfnGlDeleteShader      = nullptr;
    PFN_glCreateProgram     pfnGlCreateProgram     = nullptr;
    PFN_glAttachShader      pfnGlAttachShader      = nullptr;
    PFN_glLinkProgram       pfnGlLinkProgram       = nullptr;
    PFN_glGetProgramiv      pfnGlGetProgramiv      = nullptr;
    PFN_glGetProgramInfoLog pfnGlGetProgramInfoLog = nullptr;
    PFN_glDeleteProgram     pfnGlDeleteProgram     = nullptr;
    PFN_glUseProgram        pfnGlUseProgram        = nullptr;
    PFN_glGetUniformLocation pfnGlGetUniformLocation = nullptr;
    PFN_glUniform1ui        pfnGlUniform1ui        = nullptr;
    PFN_glGenBuffers        pfnGlGenBuffers        = nullptr;
    PFN_glDeleteBuffers     pfnGlDeleteBuffers     = nullptr;
    PFN_glBindBuffer        pfnGlBindBuffer        = nullptr;
    PFN_glBufferData        pfnGlBufferData        = nullptr;
    PFN_glBindBufferBase    pfnGlBindBufferBase    = nullptr;
    PFN_glDispatchCompute   pfnGlDispatchCompute   = nullptr;
    PFN_glMemoryBarrier     pfnGlMemoryBarrier     = nullptr;
    PFN_glGetBufferSubData  pfnGlGetBufferSubData  = nullptr;
    PFN_glGetString         pfnGlGetString         = nullptr;
    PFN_glGetIntegerv       pfnGlGetIntegerv       = nullptr;

    // Compiled compute programs
    GL_GLuint l2Program_     = 0;
    GL_GLuint cosineProgram_ = 0;

    // Device info
    std::string rendererName_;
    std::string vendorName_;
    int glMajor_ = 0;
    int glMinor_ = 0;

    bool gpuAvailable_ = false;  // true → GL context + shaders ready
    bool cpuFallback_  = false;  // true → use CPU kernels

    // ---- Library loading -----------------------------------------------

    bool loadEGLLibrary() {
#if defined(__linux__) || defined(__unix__)
        libEGL_ = openLib("libEGL.so.1");
        if (!libEGL_) libEGL_ = openLib("libEGL.so");
        libGL_  = openLib("libGL.so.1");
        if (!libGL_)  libGL_  = openLib("libGL.so");
#elif defined(_WIN32)
        libGL_  = openLib("OpenGL32.dll");
        // On Windows, ANGLE provides EGL; try common install locations
        libEGL_ = openLib("libEGL.dll");
        if (!libEGL_) libEGL_ = openLib("EGL.dll");
#endif
        return libEGL_ != nullptr;
    }

    bool loadEGLFunctions() {
        auto s = [&](const char* n) { return libSym(libEGL_, n); };
        pfnEglGetProcAddress   = reinterpret_cast<PFN_eglGetProcAddress>  (s("eglGetProcAddress"));
        pfnEglGetDisplay       = reinterpret_cast<PFN_eglGetDisplay>      (s("eglGetDisplay"));
        pfnEglInitialize       = reinterpret_cast<PFN_eglInitialize>      (s("eglInitialize"));
        pfnEglBindAPI          = reinterpret_cast<PFN_eglBindAPI>         (s("eglBindAPI"));
        pfnEglChooseConfig     = reinterpret_cast<PFN_eglChooseConfig>    (s("eglChooseConfig"));
        pfnEglCreateContext    = reinterpret_cast<PFN_eglCreateContext>   (s("eglCreateContext"));
        pfnEglMakeCurrent      = reinterpret_cast<PFN_eglMakeCurrent>     (s("eglMakeCurrent"));
        pfnEglDestroyContext   = reinterpret_cast<PFN_eglDestroyContext>  (s("eglDestroyContext"));
        pfnEglTerminate        = reinterpret_cast<PFN_eglTerminate>       (s("eglTerminate"));
        return pfnEglGetDisplay && pfnEglInitialize && pfnEglBindAPI &&
               pfnEglChooseConfig && pfnEglCreateContext && pfnEglMakeCurrent &&
               pfnEglDestroyContext && pfnEglTerminate;
    }

    void* glProc(const char* name) {
        void* fn = nullptr;
        if (pfnEglGetProcAddress) fn = pfnEglGetProcAddress(name);
        if (!fn && libGL_) fn = libSym(libGL_, name);
        return fn;
    }

    bool loadGLFunctions() {
        auto l = [&](const char* n) { return glProc(n); };
        pfnGlCreateShader       = reinterpret_cast<PFN_glCreateShader>     (l("glCreateShader"));
        pfnGlShaderSource       = reinterpret_cast<PFN_glShaderSource>     (l("glShaderSource"));
        pfnGlCompileShader      = reinterpret_cast<PFN_glCompileShader>    (l("glCompileShader"));
        pfnGlGetShaderiv        = reinterpret_cast<PFN_glGetShaderiv>      (l("glGetShaderiv"));
        pfnGlGetShaderInfoLog   = reinterpret_cast<PFN_glGetShaderInfoLog> (l("glGetShaderInfoLog"));
        pfnGlDeleteShader       = reinterpret_cast<PFN_glDeleteShader>     (l("glDeleteShader"));
        pfnGlCreateProgram      = reinterpret_cast<PFN_glCreateProgram>    (l("glCreateProgram"));
        pfnGlAttachShader       = reinterpret_cast<PFN_glAttachShader>     (l("glAttachShader"));
        pfnGlLinkProgram        = reinterpret_cast<PFN_glLinkProgram>      (l("glLinkProgram"));
        pfnGlGetProgramiv       = reinterpret_cast<PFN_glGetProgramiv>     (l("glGetProgramiv"));
        pfnGlGetProgramInfoLog  = reinterpret_cast<PFN_glGetProgramInfoLog>(l("glGetProgramInfoLog"));
        pfnGlDeleteProgram      = reinterpret_cast<PFN_glDeleteProgram>    (l("glDeleteProgram"));
        pfnGlUseProgram         = reinterpret_cast<PFN_glUseProgram>       (l("glUseProgram"));
        pfnGlGetUniformLocation = reinterpret_cast<PFN_glGetUniformLocation>(l("glGetUniformLocation"));
        pfnGlUniform1ui         = reinterpret_cast<PFN_glUniform1ui>       (l("glUniform1ui"));
        pfnGlGenBuffers         = reinterpret_cast<PFN_glGenBuffers>       (l("glGenBuffers"));
        pfnGlDeleteBuffers      = reinterpret_cast<PFN_glDeleteBuffers>    (l("glDeleteBuffers"));
        pfnGlBindBuffer         = reinterpret_cast<PFN_glBindBuffer>       (l("glBindBuffer"));
        pfnGlBufferData         = reinterpret_cast<PFN_glBufferData>       (l("glBufferData"));
        pfnGlBindBufferBase     = reinterpret_cast<PFN_glBindBufferBase>   (l("glBindBufferBase"));
        pfnGlDispatchCompute    = reinterpret_cast<PFN_glDispatchCompute>  (l("glDispatchCompute"));
        pfnGlMemoryBarrier      = reinterpret_cast<PFN_glMemoryBarrier>    (l("glMemoryBarrier"));
        pfnGlGetBufferSubData   = reinterpret_cast<PFN_glGetBufferSubData> (l("glGetBufferSubData"));
        pfnGlGetString          = reinterpret_cast<PFN_glGetString>        (l("glGetString"));
        pfnGlGetIntegerv        = reinterpret_cast<PFN_glGetIntegerv>      (l("glGetIntegerv"));
        // Minimum set needed for compute dispatch
        return pfnGlCreateShader && pfnGlShaderSource && pfnGlCompileShader &&
               pfnGlGetShaderiv && pfnGlDeleteShader && pfnGlCreateProgram &&
               pfnGlAttachShader && pfnGlLinkProgram && pfnGlGetProgramiv &&
               pfnGlDeleteProgram && pfnGlUseProgram && pfnGlGetUniformLocation &&
               pfnGlUniform1ui && pfnGlGenBuffers && pfnGlDeleteBuffers &&
               pfnGlBindBuffer && pfnGlBufferData && pfnGlBindBufferBase &&
               pfnGlDispatchCompute && pfnGlMemoryBarrier && pfnGlGetBufferSubData;
    }

    // ---- EGL context creation ------------------------------------------

    bool createEGLContext() {
        if (!pfnEglGetDisplay) return false;
        eglDisplay_ = pfnEglGetDisplay(nullptr);
        if (eglDisplay_ == k_EGL_NO_DISPLAY) return false;

        EGL_Int major = 0, minor = 0;
        if (!pfnEglInitialize(eglDisplay_, &major, &minor)) {
            eglDisplay_ = nullptr;
            return false;
        }
        if (!pfnEglBindAPI(k_EGL_OPENGL_API)) {
            pfnEglTerminate(eglDisplay_);
            eglDisplay_ = nullptr;
            return false;
        }

        const EGL_Int configAttribs[] = {
            k_EGL_RENDERABLE_TYPE, k_EGL_OPENGL_BIT,
            k_EGL_NONE
        };
        EGL_Config cfg = nullptr;
        EGL_Int numCfg = 0;
        if (!pfnEglChooseConfig(eglDisplay_, configAttribs, &cfg, 1, &numCfg)
            || numCfg == 0) {
            pfnEglTerminate(eglDisplay_);
            eglDisplay_ = nullptr;
            return false;
        }

        // Request OpenGL 4.3 Core Profile (required for compute shaders)
        const EGL_Int ctxAttribs[] = {
            k_EGL_CONTEXT_MAJOR_VERSION, 4,
            k_EGL_CONTEXT_MINOR_VERSION, 3,
            k_EGL_CONTEXT_OPENGL_PROFILE_MASK, k_EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
            k_EGL_NONE
        };
        eglContext_ = pfnEglCreateContext(eglDisplay_, cfg,
                                          k_EGL_NO_CONTEXT, ctxAttribs);
        if (eglContext_ == k_EGL_NO_CONTEXT) {
            pfnEglTerminate(eglDisplay_);
            eglDisplay_ = nullptr;
            return false;
        }

        // Surfaceless context (EGL_KHR_surfaceless_context / EGL 1.5)
        if (!pfnEglMakeCurrent(eglDisplay_,
                               k_EGL_NO_SURFACE, k_EGL_NO_SURFACE,
                               eglContext_)) {
            pfnEglDestroyContext(eglDisplay_, eglContext_);
            eglContext_ = nullptr;
            pfnEglTerminate(eglDisplay_);
            eglDisplay_ = nullptr;
            return false;
        }
        return true;
    }

    // ---- Shader compilation / linking ----------------------------------

    GL_GLuint compileShader(const char* src) {
        GL_GLuint shader = pfnGlCreateShader(k_COMPUTE_SHADER);
        const GL_GLchar* srcs[] = { src };
        pfnGlShaderSource(shader, 1, srcs, nullptr);
        pfnGlCompileShader(shader);

        GL_GLint status = 0;
        pfnGlGetShaderiv(shader, k_COMPILE_STATUS, &status);
        if (!status) {
            GL_GLint logLen = 0;
            pfnGlGetShaderiv(shader, k_INFO_LOG_LENGTH, &logLen);
            if (logLen > 0) {
                std::vector<char> log(static_cast<size_t>(logLen));
                pfnGlGetShaderInfoLog(shader, logLen, nullptr, log.data());
                std::cerr << "[OpenGL] Shader compile error: " << log.data() << std::endl;
            }
            pfnGlDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GL_GLuint linkProgram(GL_GLuint shader) {
        GL_GLuint prog = pfnGlCreateProgram();
        pfnGlAttachShader(prog, shader);
        pfnGlLinkProgram(prog);

        GL_GLint status = 0;
        pfnGlGetProgramiv(prog, k_LINK_STATUS, &status);
        if (!status) {
            GL_GLint logLen = 0;
            pfnGlGetProgramiv(prog, k_INFO_LOG_LENGTH, &logLen);
            if (logLen > 0) {
                std::vector<char> log(static_cast<size_t>(logLen));
                pfnGlGetProgramInfoLog(prog, logLen, nullptr, log.data());
                std::cerr << "[OpenGL] Program link error: " << log.data() << std::endl;
            }
            pfnGlDeleteProgram(prog);
            return 0;
        }
        return prog;
    }

    bool createShaderPrograms() {
        GL_GLuint l2Shader = compileShader(s_glsl_l2_src);
        if (!l2Shader) return false;
        l2Program_ = linkProgram(l2Shader);
        pfnGlDeleteShader(l2Shader);
        if (!l2Program_) return false;

        GL_GLuint cosShader = compileShader(s_glsl_cosine_src);
        if (!cosShader) { pfnGlDeleteProgram(l2Program_); l2Program_ = 0; return false; }
        cosineProgram_ = linkProgram(cosShader);
        pfnGlDeleteShader(cosShader);
        if (!cosineProgram_) { pfnGlDeleteProgram(l2Program_); l2Program_ = 0; return false; }

        return true;
    }

    // ---- GPU dispatch --------------------------------------------------

    // Dispatch a compute shader; SSBOs are created internally.
    // The output buffer is read back synchronously after glMemoryBarrier.
    std::vector<float> gpuDispatch(
        GL_GLuint program,
        const float* queries, uint32_t nq,
        const float* vectors, uint32_t nv,
        uint32_t dim)
    {
        const GL_GLsizeiptr qBytes = static_cast<GL_GLsizeiptr>(
            static_cast<size_t>(nq) * dim * sizeof(float));
        const GL_GLsizeiptr vBytes = static_cast<GL_GLsizeiptr>(
            static_cast<size_t>(nv) * dim * sizeof(float));
        const GL_GLsizeiptr dBytes = static_cast<GL_GLsizeiptr>(
            static_cast<size_t>(nq) * nv  * sizeof(float));

        GL_GLuint bufs[3] = {0, 0, 0};
        pfnGlGenBuffers(3, bufs);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[0]);
        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, qBytes, queries, k_STREAM_COPY);
        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[1]);
        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, vBytes, vectors, k_STREAM_COPY);
        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);
        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, dBytes, nullptr, k_DYNAMIC_COPY);
        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, 0);

        pfnGlBindBufferBase(k_SHADER_STORAGE_BUFFER, 0, bufs[0]);
        pfnGlBindBufferBase(k_SHADER_STORAGE_BUFFER, 1, bufs[1]);
        pfnGlBindBufferBase(k_SHADER_STORAGE_BUFFER, 2, bufs[2]);

        pfnGlUseProgram(program);
        GL_GLint locNQ  = pfnGlGetUniformLocation(program, "uNQ");
        GL_GLint locNV  = pfnGlGetUniformLocation(program, "uNV");
        GL_GLint locDim = pfnGlGetUniformLocation(program, "uDim");
        if (locNQ  >= 0) pfnGlUniform1ui(locNQ,  nq);
        if (locNV  >= 0) pfnGlUniform1ui(locNV,  nv);
        if (locDim >= 0) pfnGlUniform1ui(locDim, dim);

        const GL_GLuint gx = (nq + 7u) / 8u;
        const GL_GLuint gy = (nv + 7u) / 8u;
        pfnGlDispatchCompute(gx, gy, 1u);
        pfnGlMemoryBarrier(k_SHADER_STORAGE_BARRIER_BIT);

        std::vector<float> out(static_cast<size_t>(nq) * nv);
        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);
        pfnGlGetBufferSubData(k_SHADER_STORAGE_BUFFER, 0, dBytes, out.data());
        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, 0);

        pfnGlDeleteBuffers(3, bufs);
        return out;
    }

    // ---- CPU fallback kernels ------------------------------------------

    static std::vector<float> cpuL2(
        const float* queries, size_t nq, size_t dim,
        const float* vectors, size_t nv)
    {
        std::vector<float> out(nq * nv);
        for (size_t q = 0; q < nq; ++q) {
            for (size_t v = 0; v < nv; ++v) {
                float sum = 0.f;
                for (size_t d = 0; d < dim; ++d) {
                    float diff = queries[q * dim + d] - vectors[v * dim + d];
                    sum += diff * diff;
                }
                out[q * nv + v] = sum;
            }
        }
        return out;
    }

    static std::vector<float> cpuCosine(
        const float* queries, size_t nq, size_t dim,
        const float* vectors, size_t nv)
    {
        constexpr float kEps = 1e-10f;
        std::vector<float> out(nq * nv);
        for (size_t q = 0; q < nq; ++q) {
            for (size_t v = 0; v < nv; ++v) {
                float dot = 0.f, nqNorm = 0.f, nvNorm = 0.f;
                for (size_t d = 0; d < dim; ++d) {
                    float qv = queries[q * dim + d];
                    float vv = vectors[v * dim + d];
                    dot   += qv * vv;
                    nqNorm += qv * qv;
                    nvNorm += vv * vv;
                }
                float denom = std::sqrt(nqNorm) * std::sqrt(nvNorm);
                out[q * nv + v] = (denom > kEps) ? 1.f - dot / denom : 1.f;
            }
        }
        return out;
    }

    // ---- Cleanup -------------------------------------------------------

    void cleanup() {
        if (gpuAvailable_) {
            if (l2Program_)     { pfnGlDeleteProgram(l2Program_);     l2Program_ = 0; }
            if (cosineProgram_) { pfnGlDeleteProgram(cosineProgram_); cosineProgram_ = 0; }
        }
        if (eglContext_ && pfnEglMakeCurrent) {
            pfnEglMakeCurrent(eglDisplay_,
                              k_EGL_NO_SURFACE, k_EGL_NO_SURFACE,
                              k_EGL_NO_CONTEXT);
        }
        if (eglContext_ && pfnEglDestroyContext)
            pfnEglDestroyContext(eglDisplay_, eglContext_);
        if (eglDisplay_ && pfnEglTerminate)
            pfnEglTerminate(eglDisplay_);
        eglContext_ = nullptr;
        eglDisplay_ = nullptr;
        closeLib(libEGL_); libEGL_ = nullptr;
        closeLib(libGL_);  libGL_  = nullptr;
        gpuAvailable_ = false;
        cpuFallback_  = false;
    }
#else
    // Stub members so the class compiles without THEMIS_ENABLE_OPENGL
    bool gpuAvailable_ = false;
    bool cpuFallback_  = false;
    std::string rendererName_;
    std::string vendorName_;
    int glMajor_ = 0;
    int glMinor_ = 0;
    void cleanup() {}

    static std::vector<float> cpuL2(
        const float*, size_t, size_t, const float*, size_t) { return {}; }
    static std::vector<float> cpuCosine(
        const float*, size_t, size_t, const float*, size_t) { return {}; }
#endif
};

// ============================================================================
// OpenGL Vector Backend — public interface
// ============================================================================

OpenGLVectorBackend::OpenGLVectorBackend()
    : initialized_(false), impl_(std::make_unique<OpenGLVectorBackendImpl>()) {}

OpenGLVectorBackend::~OpenGLVectorBackend() {
    shutdown();
}

bool OpenGLVectorBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_OPENGL
    // Probe EGL availability by attempting a minimal OpenGL 4.3 context.
    // This is a one-shot check; we open and immediately close the library.
    void* lib = openLib("libEGL.so.1");
    if (!lib) lib = openLib("libEGL.so");
    if (!lib) return false;

    auto fnGetDisplay    = reinterpret_cast<PFN_eglGetDisplay>  (libSym(lib, "eglGetDisplay"));
    auto fnInit          = reinterpret_cast<PFN_eglInitialize>  (libSym(lib, "eglInitialize"));
    auto fnBindAPI       = reinterpret_cast<PFN_eglBindAPI>     (libSym(lib, "eglBindAPI"));
    auto fnChooseConfig  = reinterpret_cast<PFN_eglChooseConfig>(libSym(lib, "eglChooseConfig"));
    auto fnCreateContext = reinterpret_cast<PFN_eglCreateContext>(libSym(lib, "eglCreateContext"));
    auto fnDestroyCtx    = reinterpret_cast<PFN_eglDestroyContext>(libSym(lib, "eglDestroyContext"));
    auto fnTerminate     = reinterpret_cast<PFN_eglTerminate>   (libSym(lib, "eglTerminate"));

    bool ok = false;
    if (fnGetDisplay && fnInit && fnBindAPI && fnChooseConfig &&
        fnCreateContext && fnDestroyCtx && fnTerminate) {

        EGL_Display dpy = fnGetDisplay(nullptr);
        if (dpy != k_EGL_NO_DISPLAY) {
            EGL_Int major = 0, minor = 0;
            if (fnInit(dpy, &major, &minor) && fnBindAPI(k_EGL_OPENGL_API)) {
                const EGL_Int cfgAttribs[] = {
                    k_EGL_RENDERABLE_TYPE, k_EGL_OPENGL_BIT, k_EGL_NONE
                };
                EGL_Config cfg = nullptr;
                EGL_Int n = 0;
                if (fnChooseConfig(dpy, cfgAttribs, &cfg, 1, &n) && n > 0) {
                    const EGL_Int ctxAttribs[] = {
                        k_EGL_CONTEXT_MAJOR_VERSION, 4,
                        k_EGL_CONTEXT_MINOR_VERSION, 3,
                        k_EGL_CONTEXT_OPENGL_PROFILE_MASK,
                        k_EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                        k_EGL_NONE
                    };
                    EGL_Context ctx = fnCreateContext(dpy, cfg,
                                                      k_EGL_NO_CONTEXT, ctxAttribs);
                    if (ctx != k_EGL_NO_CONTEXT) {
                        ok = true;
                        fnDestroyCtx(dpy, ctx);
                    }
                }
            }
            fnTerminate(dpy);
        }
    }
    closeLib(lib);
    return ok;
#else
    return false;
#endif
}

BackendCapabilities OpenGLVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_OPENGL
    caps.supportsVectorOps       = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = false;  // OpenGL compute is synchronous
    caps.supportedPrecisions     = PrecisionMode::FP32;
    caps.supportedMetrics        = metricBit(DistanceMetric::L2)
                                 | metricBit(DistanceMetric::COSINE);
    if (initialized_ && impl_) {
        caps.deviceName = impl_->rendererName_.empty()
                        ? "OpenGL Compute" : impl_->rendererName_;
        caps.vendorName = impl_->vendorName_;
    } else {
        caps.deviceName = "OpenGL Compute";
    }
#endif
    return caps;
}

bool OpenGLVectorBackend::initialize() {
#ifdef THEMIS_ENABLE_OPENGL
    if (initialized_) return true;
    if (!impl_) impl_ = std::make_unique<OpenGLVectorBackendImpl>();

    // Attempt to create a headless OpenGL 4.3 context via EGL.
    // On failure we activate the CPU fallback so the backend remains usable.
    if (impl_->loadEGLLibrary() && impl_->loadEGLFunctions() &&
        impl_->createEGLContext() && impl_->loadGLFunctions() &&
        impl_->createShaderPrograms()) {

        // Retrieve device info
        if (impl_->pfnGlGetString) {
            const GL_GLubyte* r = impl_->pfnGlGetString(k_RENDERER);
            const GL_GLubyte* v = impl_->pfnGlGetString(k_VENDOR);
            if (r) impl_->rendererName_ = reinterpret_cast<const char*>(r);
            if (v) impl_->vendorName_   = reinterpret_cast<const char*>(v);
        }
        if (impl_->pfnGlGetIntegerv) {
            impl_->pfnGlGetIntegerv(k_MAJOR_VERSION, &impl_->glMajor_);
            impl_->pfnGlGetIntegerv(k_MINOR_VERSION, &impl_->glMinor_);
        }
        impl_->gpuAvailable_ = true;
        std::cout << "[OpenGL] Initialized: " << impl_->rendererName_
                  << " (GL " << impl_->glMajor_ << "." << impl_->glMinor_ << ")"
                  << std::endl;
    } else {
        // No EGL / GL 4.3+ available on this machine; use CPU kernels.
        // This mirrors VulkanGeoBackend which initializes successfully even
        // without a Vulkan ICD and uses CPU fallback implementations.
        std::cerr << "[OpenGL] EGL/GL 4.3+ not available; running in CPU fallback mode"
                  << std::endl;
        impl_->cpuFallback_ = true;
    }

    initialized_ = true;
    clearError();
    return true;
#else
    return false;
#endif
}

void OpenGLVectorBackend::shutdown() {
#ifdef THEMIS_ENABLE_OPENGL
    if (initialized_ && impl_) {
        impl_->cleanup();
        initialized_ = false;
    }
#endif
}

std::vector<float> OpenGLVectorBackend::computeDistances(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    bool useL2
) {
#ifdef THEMIS_ENABLE_OPENGL
    if (!initialized_ || !impl_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "OpenGL",
            "OpenGL backend not initialized",
            "Call initialize() before using the backend"));
        return {};
    }
    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "OpenGL", AccelerationErrorCode::InvalidInputShape,
            "queries and vectors pointers must be non-null"));
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0) {
        setError(ErrorContextHelpers::createValidationError(
            "OpenGL", AccelerationErrorCode::InvalidInputShape,
            "numQueries, numVectors, and dim must all be > 0"));
        return {};
    }

    try {
        if (impl_->gpuAvailable_) {
            GL_GLuint prog = useL2 ? impl_->l2Program_ : impl_->cosineProgram_;
            auto result = impl_->gpuDispatch(
                prog, queries, static_cast<uint32_t>(numQueries),
                vectors, static_cast<uint32_t>(numVectors),
                static_cast<uint32_t>(dim));
            clearError();
            return result;
        } else {
            // CPU fallback
            auto result = useL2
                ? OpenGLVectorBackendImpl::cpuL2(queries, numQueries, dim, vectors, numVectors)
                : OpenGLVectorBackendImpl::cpuCosine(queries, numQueries, dim, vectors, numVectors);
            clearError();
            return result;
        }
    } catch (const std::exception& e) {
        std::cerr << "[OpenGL] computeDistances error: " << e.what() << std::endl;
        setError(ErrorContext(
            AccelerationErrorCode::AllocationFailed,
            "OpenGL",
            std::string("Dispatch failed: ") + e.what(),
            "Reduce batch size or check GPU memory"));
        return {};
    }
#else
    return {};
#endif
}

std::vector<std::vector<std::pair<uint32_t, float>>> OpenGLVectorBackend::batchKnnSearch(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    size_t k,
    bool useL2
) {
#ifdef THEMIS_ENABLE_OPENGL
    if (!initialized_ || !impl_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "OpenGL",
            "OpenGL backend not initialized",
            "Call initialize() before using the backend"));
        return {};
    }
    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "OpenGL", AccelerationErrorCode::InvalidInputShape,
            "queries and vectors pointers must be non-null"));
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0 || k == 0) {
        setError(ErrorContextHelpers::createValidationError(
            "OpenGL", AccelerationErrorCode::InvalidInputShape,
            "numQueries, numVectors, dim, and k must all be > 0"));
        return {};
    }

    // Compute pairwise distances (GPU or CPU fallback)
    std::vector<float> distances;
    try {
        if (impl_->gpuAvailable_) {
            GL_GLuint prog = useL2 ? impl_->l2Program_ : impl_->cosineProgram_;
            distances = impl_->gpuDispatch(
                prog, queries, static_cast<uint32_t>(numQueries),
                vectors, static_cast<uint32_t>(numVectors),
                static_cast<uint32_t>(dim));
        } else {
            distances = useL2
                ? OpenGLVectorBackendImpl::cpuL2(queries, numQueries, dim, vectors, numVectors)
                : OpenGLVectorBackendImpl::cpuCosine(queries, numQueries, dim, vectors, numVectors);
        }
    } catch (const std::exception& e) {
        std::cerr << "[OpenGL] batchKnnSearch dispatch error: " << e.what() << std::endl;
        setError(ErrorContext(
            AccelerationErrorCode::AllocationFailed,
            "OpenGL",
            std::string("Dispatch failed: ") + e.what(),
            "Reduce batch size or check GPU memory"));
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
