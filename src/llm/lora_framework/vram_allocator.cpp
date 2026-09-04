/**
 * @file vram_allocator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=54, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/vram_allocator.h"
#include "security/vram_secure_clear.h"
#include "themis/gpu/memory_manager.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>  // For posix_memalign
#endif

// Backend-specific includes (conditionally compiled)
#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

#ifdef THEMIS_ENABLE_VULKAN
#include <vulkan/vulkan.h>
#endif

namespace themis {
namespace llm {
namespace lora {

namespace {
    // Helper to align size up to alignment boundary
    constexpr size_t align_up(size_t size, size_t alignment) {
        return ((size + alignment - 1) / alignment) * alignment;
    }

    // Return true when `backend` maps to real GPU hardware (VRAM) so that
    // the canonical themis::gpu::GPUMemoryManager policy should be consulted.
    constexpr bool is_gpu_backend(acceleration::BackendType backend) noexcept {
        switch (backend) {
            case acceleration::BackendType::CUDA:
            [[fallthrough]];\n            case acceleration::BackendType::HIP:
            [[fallthrough]];\n            case acceleration::BackendType::VULKAN:
            [[fallthrough]];\n            case acceleration::BackendType::DIRECTX:
            [[fallthrough]];\n            case acceleration::BackendType::ROCM:
            [[fallthrough]];\n            case acceleration::BackendType::ZLUDA:
                return true;
            default:
                return false;
        }
    }

#ifdef THEMIS_ENABLE_VULKAN
    // ---------------------------------------------------------------------------
    // VulkanAllocContext – holds all Vulkan handles created for this allocator.
    //
    // Memory model: every allocation maps a VkBuffer backed by
    // HOST_VISIBLE | HOST_COHERENT device memory.  This makes upload/download
    // trivial (plain memcpy through the persistently-mapped pointer) at the
    // cost of bypassing GPU-local (DEVICE_LOCAL) bandwidth.  For a training
    // workload the GPU operates on this buffer via vkCmdCopyBuffer to a
    // device-local staging target; that path is handled by the caller.
    // ---------------------------------------------------------------------------
    struct VulkanAllocContext {
        VkInstance       instance        = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VkDevice         device          = VK_NULL_HANDLE;
        uint32_t         memory_type_idx = UINT32_MAX;

        // Per-allocation tracking (buffer + memory + mapped host pointer).
        struct AllocEntry {
            VkBuffer       buffer   = VK_NULL_HANDLE;
            VkDeviceMemory memory   = VK_NULL_HANDLE;
            void*          mapped   = nullptr;
            VkDeviceSize   size     = 0;
        };
        std::vector<AllocEntry> entries;

        // Find a memory type that satisfies requiredBits and has the requested
        // property flags.  Returns UINT32_MAX on failure.
        uint32_t findMemoryType(uint32_t type_bits,
                                VkMemoryPropertyFlags props) const {
            VkPhysicalDeviceMemoryProperties mem_props{};
            vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
            for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
                if ((type_bits & (1u << i)) &&
                    (mem_props.memoryTypes[i].propertyFlags & props) == props) {
                    return i;
                }
            }
            return UINT32_MAX;
        }
    };

    static bool vk_init(VulkanAllocContext* ctx, size_t& pool_size_out) {
        // 1. Instance
        VkApplicationInfo app_info{};
        app_info.sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pEngineName = "ThemisDB";
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo inst_ci{};
        inst_ci.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_ci.pApplicationInfo = &app_info;

        if (vkCreateInstance(&inst_ci, nullptr, &ctx->instance) != VK_SUCCESS) {
            spdlog::error("VRAMAllocator(Vulkan): vkCreateInstance failed");
            return false;
        }

        // 2. Physical device (first discrete GPU, or first available)
        uint32_t dev_count = 0;
        {
            VkResult enum_result = vkEnumeratePhysicalDevices(ctx->instance, &dev_count, nullptr);
            if (enum_result != VK_SUCCESS && enum_result != VK_INCOMPLETE) {
                spdlog::error("VRAMAllocator(Vulkan): vkEnumeratePhysicalDevices (count) failed: {}",
                              static_cast<int>(enum_result));
                vkDestroyInstance(ctx->instance, nullptr);
                ctx->instance = VK_NULL_HANDLE;
                return false;
            }
        }
        if (dev_count == 0) {
            spdlog::error("VRAMAllocator(Vulkan): no physical devices found");
            vkDestroyInstance(ctx->instance, nullptr);
            ctx->instance = VK_NULL_HANDLE;
            return false;
        }
        std::vector<VkPhysicalDevice> devs(dev_count);
        {
            VkResult enum_result = vkEnumeratePhysicalDevices(ctx->instance, &dev_count, devs.data());
            if (enum_result != VK_SUCCESS && enum_result != VK_INCOMPLETE) {
                spdlog::error("VRAMAllocator(Vulkan): vkEnumeratePhysicalDevices (fill) failed: {}",
                              static_cast<int>(enum_result));
                vkDestroyInstance(ctx->instance, nullptr);
                ctx->instance = VK_NULL_HANDLE;
                return false;
            }
        }

        ctx->physical_device = devs[0];                     // fallback
        for (const auto& pd : devs) {
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(pd, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                ctx->physical_device = pd;
                break;
            }
        }

        // 3. Queue family (first that supports compute)
        uint32_t qf_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &qf_count, nullptr);
        std::vector<VkQueueFamilyProperties> qf_props(qf_count);
        vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &qf_count,
                                                 qf_props.data());

        uint32_t queue_family = UINT32_MAX;
        for (uint32_t i = 0; i < qf_count; ++i) {
            if (qf_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                queue_family = i;
                break;
            }
        }
        if (queue_family == UINT32_MAX) {
            spdlog::error("VRAMAllocator(Vulkan): no compute queue family");
            vkDestroyInstance(ctx->instance, nullptr);
            ctx->instance = VK_NULL_HANDLE;
            return false;
        }

        // 4. Logical device
        float prio = 1.0f;
        VkDeviceQueueCreateInfo q_ci{};
        q_ci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q_ci.queueFamilyIndex = queue_family;
        q_ci.queueCount       = 1;
        q_ci.pQueuePriorities = &prio;

        VkDeviceCreateInfo dev_ci{};
        dev_ci.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dev_ci.queueCreateInfoCount = 1;
        dev_ci.pQueueCreateInfos    = &q_ci;

        if (vkCreateDevice(ctx->physical_device, &dev_ci, nullptr,
                           &ctx->device) != VK_SUCCESS) {
            spdlog::error("VRAMAllocator(Vulkan): vkCreateDevice failed");
            vkDestroyInstance(ctx->instance, nullptr);
            ctx->instance = VK_NULL_HANDLE;
            return false;
        }

        // 5. Determine available device-local memory for pool sizing.
        VkPhysicalDeviceMemoryProperties mem_props{};
        vkGetPhysicalDeviceMemoryProperties(ctx->physical_device, &mem_props);
        VkDeviceSize total_host_visible = 0;
        for (uint32_t i = 0; i < mem_props.memoryHeapCount; ++i) {
            if (mem_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                total_host_visible = mem_props.memoryHeaps[i].size;
                break;
            }
        }
        if (total_host_visible > 0) {
            pool_size_out = static_cast<size_t>(total_host_visible * 0.8);
        }

        spdlog::info("VRAMAllocator(Vulkan): initialised (device-local heap {} MB, "
                     "pool {} MB)",
                     total_host_visible / (1024 * 1024),
                     pool_size_out / (1024 * 1024));
        return true;
    }

    static void* vk_alloc(VulkanAllocContext* ctx, size_t size_bytes) {
        // Create a HOST_VISIBLE | HOST_COHERENT buffer so the host can
        // memcpy into it directly without an explicit flush/invalidate.
        VkBufferCreateInfo buf_ci{};
        buf_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_ci.size        = static_cast<VkDeviceSize>(size_bytes);
        buf_ci.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT  |
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer = VK_NULL_HANDLE;
        if (vkCreateBuffer(ctx->device, &buf_ci, nullptr, &buffer) != VK_SUCCESS) {
            spdlog::error("VRAMAllocator(Vulkan): vkCreateBuffer failed ({} bytes)",
                          size_bytes);
            return nullptr;
        }

        VkMemoryRequirements mem_req{};
        vkGetBufferMemoryRequirements(ctx->device, buffer, &mem_req);

        constexpr VkMemoryPropertyFlags kHostProps =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        const uint32_t mem_type =
            ctx->findMemoryType(mem_req.memoryTypeBits, kHostProps);
        if (mem_type == UINT32_MAX) {
            spdlog::error("VRAMAllocator(Vulkan): no suitable host-visible memory type");
            vkDestroyBuffer(ctx->device, buffer, nullptr);
            return nullptr;
        }

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_req.size;
        alloc_info.memoryTypeIndex = mem_type;

        VkDeviceMemory memory = VK_NULL_HANDLE;
        if (vkAllocateMemory(ctx->device, &alloc_info, nullptr, &memory) != VK_SUCCESS) {
            spdlog::error("VRAMAllocator(Vulkan): vkAllocateMemory failed");
            vkDestroyBuffer(ctx->device, buffer, nullptr);
            return nullptr;
        }

        if (vkBindBufferMemory(ctx->device, buffer, memory, 0) != VK_SUCCESS) {
            spdlog::error("VRAMAllocator(Vulkan): vkBindBufferMemory failed");
            vkFreeMemory(ctx->device, memory, nullptr);
            vkDestroyBuffer(ctx->device, buffer, nullptr);
            return nullptr;
        }

        void* mapped = nullptr;
        if (vkMapMemory(ctx->device, memory, 0, mem_req.size, 0, &mapped) != VK_SUCCESS) {
            spdlog::error("VRAMAllocator(Vulkan): vkMapMemory failed");
            vkFreeMemory(ctx->device, memory, nullptr);
            vkDestroyBuffer(ctx->device, buffer, nullptr);
            return nullptr;
        }

        ctx->entries.push_back({buffer, memory, mapped,
                                static_cast<VkDeviceSize>(size_bytes)});
        return mapped;
    }

    static void vk_free(VulkanAllocContext* ctx, void* ptr) {
        for (auto it = ctx->entries.begin(); it != ctx->entries.end(); ++it) {
            if (it->mapped == ptr) {
                vkUnmapMemory(ctx->device, it->memory);
                vkFreeMemory(ctx->device, it->memory, nullptr);
                vkDestroyBuffer(ctx->device, it->buffer, nullptr);
                ctx->entries.erase(it);
                return;
            }
        }
        spdlog::warn("VRAMAllocator(Vulkan): vk_free: unknown pointer {:p}", ptr);
    }

    static void vk_shutdown(VulkanAllocContext* ctx) {
        // Unmap and free any remaining allocations.
        for (auto& e : ctx->entries) {
            if (e.mapped) {
              vkUnmapMemory(ctx->device, e.memory);
            }
            if (e.memory != VK_NULL_HANDLE) {
              vkFreeMemory(ctx->device, e.memory, nullptr);
            }
            if (e.buffer != VK_NULL_HANDLE) {
              vkDestroyBuffer(ctx->device, e.buffer, nullptr);
            }
        }
        ctx->entries.clear();
        if (ctx->device   != VK_NULL_HANDLE) {
          vkDestroyDevice(ctx->device, nullptr);
        }
        if (ctx->instance != VK_NULL_HANDLE) {
          vkDestroyInstance(ctx->instance, nullptr);
        }
        ctx->device   = VK_NULL_HANDLE;
        ctx->instance = VK_NULL_HANDLE;
    }
#endif // THEMIS_ENABLE_VULKAN

} // anonymous namespace

// ============================================================================
// VRAMAllocator Implementation
// ============================================================================

VRAMAllocator::VRAMAllocator(acceleration::BackendType backend, size_t pool_size_bytes)
    : backend_(backend), pool_size_bytes_(pool_size_bytes) {
    
    // Auto-detect pool size if not specified (use 80% of currently free VRAM).
    // Using free memory rather than total capacity avoids OOM when other
    // processes already occupy part of the device.
    if (pool_size_bytes_ == 0) {
#ifdef THEMIS_ENABLE_CUDA
        if (backend_ == acceleration::BackendType::CUDA) {
            size_t free_bytes = 0, total_bytes = 0;
            if (cudaMemGetInfo(&free_bytes, &total_bytes) == cudaSuccess && free_bytes > 0) {
                pool_size_bytes_ = static_cast<size_t>(free_bytes * 0.8);
                spdlog::info("VRAMAllocator: CUDA free={} MB total={} MB, reserving {} MB (80% of free)",
                             free_bytes / (1024 * 1024), total_bytes / (1024 * 1024),
                             pool_size_bytes_ / (1024 * 1024));
            }
        }
#endif
#ifdef THEMIS_ENABLE_HIP
        if (backend_ == acceleration::BackendType::HIP) {
            size_t free_bytes = 0, total_bytes = 0;
            if (hipMemGetInfo(&free_bytes, &total_bytes) == hipSuccess && free_bytes > 0) {
                pool_size_bytes_ = static_cast<size_t>(free_bytes * 0.8);
                spdlog::info("VRAMAllocator: HIP free={} MB total={} MB, reserving {} MB (80% of free)",
                             free_bytes / (1024 * 1024), total_bytes / (1024 * 1024),
                             pool_size_bytes_ / (1024 * 1024));
            }
        }
#endif
        if (pool_size_bytes_ == 0) {
            pool_size_bytes_ = 8 * 1024 * 1024 * 1024; // Default 8 GB fallback
            spdlog::debug("VRAMAllocator: could not query backend memory, defaulting to 8 GB pool");
        }
    }
    
    initialized_ = initialize_backend();
}

VRAMAllocator::~VRAMAllocator() {
    reset();
    shutdown_backend();
}

VRAMAllocator::VRAMAllocator(VRAMAllocator&& other) noexcept
    : backend_(other.backend_)
    , initialized_(other.initialized_)
    , memory_pool_(std::move(other.memory_pool_))
    , pool_size_bytes_(other.pool_size_bytes_)
    , allocated_bytes_(other.allocated_bytes_)
    , peak_usage_bytes_(other.peak_usage_bytes_)
    , backend_context_(other.backend_context_) {
    
    other.initialized_ = false;
    other.backend_context_ = nullptr;
    other.pool_size_bytes_ = 0;
    other.allocated_bytes_ = 0;
}

VRAMAllocator& VRAMAllocator::operator=(VRAMAllocator&& other) noexcept {
    if (this != &other) {
        reset();
        shutdown_backend();
        
        backend_ = other.backend_;
        initialized_ = other.initialized_;
        memory_pool_ = std::move(other.memory_pool_);
        pool_size_bytes_ = other.pool_size_bytes_;
        allocated_bytes_ = other.allocated_bytes_;
        peak_usage_bytes_ = other.peak_usage_bytes_;
        backend_context_ = other.backend_context_;
        
        other.initialized_ = false;
        other.backend_context_ = nullptr;
        other.pool_size_bytes_ = 0;
        other.allocated_bytes_ = 0;
    }
    return *this;
}

bool VRAMAllocator::initialize_backend() {
    switch (backend_) {
#ifdef THEMIS_ENABLE_CUDA
        case acceleration::BackendType::CUDA: {
            // Initialize CUDA
            int device_count = 0;
            cudaError_t err = cudaGetDeviceCount(&device_count);
            if (err != cudaSuccess || device_count == 0) {
                spdlog::error("CUDA initialization failed: {} (device count: {})", 
                             cudaGetErrorString(err), device_count);
                return false;
            }
            
            // Set device 0 as default
            err = cudaSetDevice(0);
            if (err != cudaSuccess) {
                spdlog::error("Failed to set CUDA device 0: {}", cudaGetErrorString(err));
                return false;
            }
            
            // Query available memory
            size_t free_bytes, total_bytes;
            err = cudaMemGetInfo(&free_bytes, &total_bytes);
            if (err != cudaSuccess) {
                spdlog::error("Failed to query CUDA memory info: {}", cudaGetErrorString(err));
                return false;
            }
            
            // Use 80% of free memory if pool_size not specified
            if (pool_size_bytes_ == 0 || pool_size_bytes_ > free_bytes) {
                pool_size_bytes_ = static_cast<size_t>(free_bytes * 0.8);
            }
            
            return true;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        case acceleration::BackendType::HIP: {
            // Initialize HIP
            int device_count = 0;
            hipError_t err = hipGetDeviceCount(&device_count);
            if (err != hipSuccess || device_count == 0) {
                spdlog::error("HIP initialization failed: {} (device count: {})", 
                             hipGetErrorString(err), device_count);
                return false;
            }
            
            err = hipSetDevice(0);
            if (err != hipSuccess) {
                spdlog::error("Failed to set HIP device 0: {}", hipGetErrorString(err));
                return false;
            }
            
            size_t free_bytes, total_bytes;
            err = hipMemGetInfo(&free_bytes, &total_bytes);
            if (err != hipSuccess) {
                spdlog::error("Failed to query HIP memory info: {}", hipGetErrorString(err));
                return false;
            }
            
            if (pool_size_bytes_ == 0 || pool_size_bytes_ > free_bytes) {
                pool_size_bytes_ = static_cast<size_t>(free_bytes * 0.8);
            }
            
            return true;
        }
#endif
        
        case acceleration::BackendType::VULKAN:
#ifdef THEMIS_ENABLE_VULKAN
        {
            // REL-48: use RAII unique_ptr instead of raw new/delete for Vulkan context
            auto vk_ctx_owner = std::make_unique<VulkanAllocContext>();
            if (!vk_init(vk_ctx_owner.get(), pool_size_bytes_)) {
                return false;
            }
            backend_context_ = vk_ctx_owner.release();
            return true;
        }
#else
            spdlog::warn("VRAMAllocator: Vulkan requested but THEMIS_ENABLE_VULKAN not set");
            return false;
#endif

        case acceleration::BackendType::DIRECTX:
            // DirectX 12 resource heaps require a D3D12Device which is not
            // available at this layer without a display subsystem.  For
            // headless compute workloads (training) use CUDA or Vulkan
            // instead.  CPU fallback is always available.
            spdlog::warn("VRAMAllocator: DirectX backend not supported in headless mode; "
                         "use CUDA, HIP, or Vulkan");
            return false;

        case acceleration::BackendType::CPU:
            // These backends require more complex initialization
            // For now, mark as available but with limited functionality
            return true;
            
        default:
            return false;
    }
}

void VRAMAllocator::shutdown_backend() {
#ifdef THEMIS_ENABLE_VULKAN
    if (backend_ == acceleration::BackendType::VULKAN && backend_context_) {
        auto* vk_ctx = static_cast<VulkanAllocContext*>(backend_context_);
        vk_shutdown(vk_ctx);
        delete vk_ctx;
    }
#endif
    backend_context_ = nullptr;
}

void* VRAMAllocator::allocate(size_t size_bytes, size_t alignment) {
    if (!initialized_ || size_bytes == 0) {
        return nullptr;
    }
    if (pool_size_bytes_ > 0 && size_bytes > pool_size_bytes_) {
        spdlog::error("VRAMAllocator::allocate: requested {} bytes exceeds pool size {} bytes",
                      size_bytes, pool_size_bytes_);
        return nullptr;
    }

    // Gate GPU allocations through the canonical policy (edition limits + tenant quotas).
    const bool use_canonical = is_gpu_backend(backend_);
    if (use_canonical) {
        auto& policy = themis::gpu::GPUMemoryManager::GetInstance();
        if (policy.isGPUEnabled() &&
            !policy.TryAllocateGPU(static_cast<uint64_t>(size_bytes), "lora-vram")) {
            spdlog::error("VRAMAllocator::allocate: canonical VRAM policy rejected {} bytes", size_bytes);
            return nullptr;
        }
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Align size
    size_bytes = align_up(size_bytes, alignment);
    
    // Try to find a free block in the pool first
    VRAMBlock* block = find_free_block(size_bytes, alignment);
    if (block != nullptr) {
        block->is_free = false;
        allocated_bytes_ += block->size;
        peak_usage_bytes_ = std::max(peak_usage_bytes_, allocated_bytes_);
        return block->ptr;
    }
    
    // Allocate new block from backend
    void* ptr = allocate_from_backend(size_bytes, alignment);
    if (ptr == nullptr) {
        // Undo canonical reservation on physical allocation failure.
        if (use_canonical) {
            auto& policy = themis::gpu::GPUMemoryManager::GetInstance();
            if (policy.isGPUEnabled()) {
                policy.DeallocateGPU(static_cast<uint64_t>(size_bytes));
            }
        }
        return nullptr;
    }
    
    // Add to memory pool
    VRAMBlock new_block;
    new_block.ptr = ptr;
    new_block.size = size_bytes;
    new_block.is_free = false;
    new_block.alignment = alignment;
    memory_pool_.push_back(new_block);
    
    allocated_bytes_ += size_bytes;
    peak_usage_bytes_ = std::max(peak_usage_bytes_, allocated_bytes_);
    
    return ptr;
}

void VRAMAllocator::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find block in pool
    for (auto& block : memory_pool_) {
        if (block.ptr == ptr) {
            if (block.is_free) {
                // Double free - ignore
                return;
            }
            block.is_free = true;
            allocated_bytes_ -= block.size;
            
            // Notify the canonical policy that this VRAM is no longer in use.
            if (is_gpu_backend(backend_)) {
                auto& policy = themis::gpu::GPUMemoryManager::GetInstance();
                if (policy.isGPUEnabled()) {
                    policy.DeallocateGPU(static_cast<uint64_t>(block.size));
                }
            }

            // Periodically coalesce free blocks
            if (static_cast<int>(memory_pool_.size()) > 100) {
                coalesce_free_blocks();
            }
            return;
        }
    }
    
    // Not in pool - direct backend deallocation
    deallocate_to_backend(ptr);
}

bool VRAMAllocator::upload(void* dst, const void* src, size_t size_bytes) {
    if (!initialized_ || dst == nullptr || src == nullptr || size_bytes == 0) {
        return false;
    }
    
    switch (backend_) {
#ifdef THEMIS_ENABLE_CUDA
        case acceleration::BackendType::CUDA: {
            cudaError_t err = cudaMemcpy(dst, src, size_bytes, cudaMemcpyHostToDevice);
            return err == cudaSuccess;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        case acceleration::BackendType::HIP: {
            hipError_t err = hipMemcpy(dst, src, size_bytes, hipMemcpyHostToDevice);
            return err == hipSuccess;
        }
#endif
        
        case acceleration::BackendType::VULKAN:
#ifdef THEMIS_ENABLE_VULKAN
            // Vulkan allocations use HOST_VISIBLE | HOST_COHERENT memory, so
            // the mapped pointer is directly accessible from the host.
            // "Upload" is therefore a plain memcpy into the mapped buffer.
            if (dst && src && size_bytes > 0) {
                std::memcpy(dst, src, size_bytes);
                return true;
            }
            return false;
#else
            return false;
#endif

        case acceleration::BackendType::DIRECTX:
            return false;

        case acceleration::BackendType::CPU:
            // CPU "upload" is just a memcpy
            std::memcpy(dst, src, size_bytes);
            return true;
            
        default:
            return false;
    }
}

bool VRAMAllocator::download(void* dst, const void* src, size_t size_bytes) {
    if (!initialized_ || dst == nullptr || src == nullptr || size_bytes == 0) {
        return false;
    }
    
    switch (backend_) {
#ifdef THEMIS_ENABLE_CUDA
        case acceleration::BackendType::CUDA: {
            cudaError_t err = cudaMemcpy(dst, src, size_bytes, cudaMemcpyDeviceToHost);
            return err == cudaSuccess;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        case acceleration::BackendType::HIP: {
            hipError_t err = hipMemcpy(dst, src, size_bytes, hipMemcpyDeviceToHost);
            return err == hipSuccess;
        }
#endif
        
        case acceleration::BackendType::VULKAN:
#ifdef THEMIS_ENABLE_VULKAN
            // HOST_VISIBLE | HOST_COHERENT memory: download is also a plain memcpy.
            if (dst && src && size_bytes > 0) {
                std::memcpy(dst, src, size_bytes);
                return true;
            }
            return false;
#else
            return false;
#endif

        case acceleration::BackendType::DIRECTX:
            return false;
            
        case acceleration::BackendType::CPU:
            // CPU "download" is just a memcpy
            std::memcpy(dst, src, size_bytes);
            return true;
            
        default:
            return false;
    }
}

VRAMAllocator::Stats VRAMAllocator::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.total_bytes = pool_size_bytes_;
    stats.allocated_bytes = allocated_bytes_;
    stats.free_bytes = pool_size_bytes_ - allocated_bytes_;
    stats.peak_usage_bytes = peak_usage_bytes_;
    stats.allocation_count = memory_pool_.size();
    
    // Calculate overhead (block metadata)
    stats.overhead_bytes = memory_pool_.size() * sizeof(VRAMBlock);
    
    // Calculate fragmentation
    size_t largest_free_block = 0;
    size_t total_free = 0;
    for (const auto& block : memory_pool_) {
        if (block.is_free) {
            largest_free_block = std::max(largest_free_block, block.size);
            total_free += block.size;
        }
    }
    
    if (total_free > 0) {
        stats.fragmentation = 1.0f - (static_cast<float>(largest_free_block) / total_free);
    }
    
    return stats;
}

void VRAMAllocator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Free all allocated blocks using the non-locking helper so that we do
    // not attempt to re-acquire mutex_ while already holding it.
    for (auto& block : memory_pool_) {
        if (block.ptr != nullptr) {
            release_backend_ptr_(block.ptr, block.size);
        }
    }

    memory_pool_.clear();
    allocated_bytes_ = 0;
}

void* VRAMAllocator::allocate_from_backend(size_t size_bytes, size_t alignment) {
    void* ptr = nullptr;
    
    switch (backend_) {
#ifdef THEMIS_ENABLE_CUDA
        case acceleration::BackendType::CUDA: {
            cudaError_t err = cudaMalloc(&ptr, size_bytes);
            if (err != cudaSuccess) {
                spdlog::error("CUDA allocation failed for {} bytes: {}", 
                             size_bytes, cudaGetErrorString(err));
                return nullptr;
            }
            if (ptr == nullptr) {
                spdlog::error("CUDA allocation returned null pointer for {} bytes despite success code", 
                             size_bytes);
                return nullptr;
            }
            return ptr;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        case acceleration::BackendType::HIP: {
            hipError_t err = hipMalloc(&ptr, size_bytes);
            if (err != hipSuccess) {
                spdlog::error("HIP allocation failed for {} bytes: {}", 
                             size_bytes, hipGetErrorString(err));
                return nullptr;
            }
            if (ptr == nullptr) {
                spdlog::error("HIP allocation returned null pointer for {} bytes despite success code", 
                             size_bytes);
                return nullptr;
            }
            return ptr;
        }
#endif
        
        case acceleration::BackendType::VULKAN:
#ifdef THEMIS_ENABLE_VULKAN
            if (backend_context_) {
                auto* vk_ctx = static_cast<VulkanAllocContext*>(backend_context_);
                void* mapped = vk_alloc(vk_ctx, size_bytes);
                return mapped;         // mapped pointer is the "device" handle
            }
            spdlog::error("VRAMAllocator(Vulkan): backend context not initialised");
            return nullptr;
#else
            spdlog::warn("VRAM allocation: Vulkan requested but THEMIS_ENABLE_VULKAN not set");
            return nullptr;
#endif

        case acceleration::BackendType::DIRECTX:
            spdlog::warn("VRAM allocation: DirectX not supported in headless mode");
            return nullptr;
            
        case acceleration::BackendType::CPU:
            // CPU allocation with alignment
#ifdef _WIN32
            ptr = _aligned_malloc(size_bytes, alignment);
            if (ptr == nullptr) {
                spdlog::error("CPU aligned allocation failed for {} bytes with alignment {}", 
                             size_bytes, alignment);
            }
#else
            if (posix_memalign(&ptr, alignment, size_bytes) != 0) {
                spdlog::error("CPU posix_memalign failed for {} bytes with alignment {}", 
                             size_bytes, alignment);
                ptr = nullptr;
            }
#endif
            return ptr;
            
        default:
            spdlog::error("Unknown backend type for VRAM allocation");
            return nullptr;
    }
}

void VRAMAllocator::release_backend_ptr_(void* ptr, size_t block_size) noexcept {
    // Performs the actual backend-specific free without holding mutex_.
    // Callers are responsible for any pool bookkeeping.
    if (ptr == nullptr) {
      return;
    }

    switch (backend_) {
#ifdef THEMIS_ENABLE_CUDA
        case acceleration::BackendType::CUDA:
            if (block_size > 0) {
                security::VRAMSecureClear::secureClearCUDA(ptr, block_size);
            }
            // REL-64: check cudaFree return value in release_backend_ptr_
            {
                cudaError_t free_err = cudaFree(ptr);
                if (free_err != cudaSuccess) {
                    spdlog::error("VRAMAllocator::release_backend_ptr_: cudaFree failed: {}",
                                  cudaGetErrorString(free_err));
                }
            }
            break;
#endif

#ifdef THEMIS_ENABLE_HIP
        case acceleration::BackendType::HIP:
            if (block_size > 0) {
                security::VRAMSecureClear::secureClearHIP(ptr, block_size);
            }
            // REL-65: check hipFree return value in release_backend_ptr_
            {
                hipError_t free_err = hipFree(ptr);
                if (free_err != hipSuccess) {
                    spdlog::error("VRAMAllocator::release_backend_ptr_: hipFree failed: {}",
                                  hipGetErrorString(free_err));
                }
            }
            break;
#endif

        case acceleration::BackendType::VULKAN:
#ifdef THEMIS_ENABLE_VULKAN
            if (backend_context_ && ptr) {
                auto* vk_ctx = static_cast<VulkanAllocContext*>(backend_context_);
                if (block_size > 0) {
                    // Secure-clear the host-visible mapped memory before releasing.
                    security::VRAMSecureClear::secureClearCPU(ptr, block_size);
                }
                vk_free(vk_ctx, ptr);
            }
            break;
#else
            break;
#endif

        case acceleration::BackendType::DIRECTX:
            break;

        case acceleration::BackendType::CPU:
            if (block_size > 0) {
                security::VRAMSecureClear::secureClearCPU(ptr, block_size);
            }
#ifdef _WIN32
            _aligned_free(ptr);
#else
            free(ptr);
#endif
            break;

        default:
            break;
    }
}

void VRAMAllocator::deallocate_to_backend(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    // Find the block size for secure clearing while holding the lock,
    // then release the lock before calling release_backend_ptr_() so that
    // re-entrant callers (reset, coalesce_free_blocks) can use the
    // non-locking helper directly and avoid recursive locking.
    size_t block_size = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& block : memory_pool_) {
            if (block.ptr == ptr) {
                block_size = block.size;
                break;
            }
        }
    }

    release_backend_ptr_(ptr, block_size);
}

VRAMBlock* VRAMAllocator::find_free_block(size_t size_bytes, size_t alignment) {
    VRAMBlock* best_fit = nullptr;
    size_t smallest_fit = SIZE_MAX;
    
    for (auto& block : memory_pool_) {
        if (block.is_free && block.size >= size_bytes && block.alignment >= alignment) {
            if (block.size < smallest_fit) {
                best_fit = &block;
                smallest_fit = block.size;
            }
        }
    }
    
    return best_fit;
}

void VRAMAllocator::coalesce_free_blocks() {
    // Remove and actually free blocks that have been marked free for a while
    auto it = memory_pool_.begin();
    while (it != memory_pool_.end()) {
        if (it->is_free) {
            deallocate_to_backend(it->ptr);
            it = memory_pool_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// VRAMTensor Implementation
// ============================================================================

VRAMTensor::VRAMTensor(VRAMAllocator* allocator, size_t size_bytes)
    : allocator_(allocator), size_(size_bytes) {
    
    if (allocator_ != nullptr) {
        ptr_ = allocator_->allocate(size_bytes);
    }
}

VRAMTensor::~VRAMTensor() {
    if (allocator_ != nullptr && ptr_ != nullptr) {
        allocator_->deallocate(ptr_);
    }
}

VRAMTensor::VRAMTensor(VRAMTensor&& other) noexcept
    : allocator_(other.allocator_)
    , ptr_(other.ptr_)
    , size_(other.size_) {
    
    other.allocator_ = nullptr;
    other.ptr_ = nullptr;
    other.size_ = 0;
}

VRAMTensor& VRAMTensor::operator=(VRAMTensor&& other) noexcept {
    if (this != &other) {
        if (allocator_ != nullptr && ptr_ != nullptr) {
            allocator_->deallocate(ptr_);
        }
        
        allocator_ = other.allocator_;
        ptr_ = other.ptr_;
        size_ = other.size_;
        
        other.allocator_ = nullptr;
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

bool VRAMTensor::upload(const void* src, size_t size_bytes) {
    if (allocator_ == nullptr || ptr_ == nullptr || src == nullptr) {
        return false;
    }
    
    if (size_bytes > size_) {
        return false;
    }
    
    return allocator_->upload(ptr_, src, size_bytes);
}

bool VRAMTensor::download(void* dst, size_t size_bytes) const {
    if (allocator_ == nullptr || ptr_ == nullptr || dst == nullptr) {
        return false;
    }
    
    if (size_bytes > size_) {
        return false;
    }
    
    return allocator_->download(dst, ptr_, size_bytes);
}

} // namespace lora
} // namespace llm
} // namespace themis
