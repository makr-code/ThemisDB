/**
 * @file gpu_memory_allocator.cpp
 * @brief Implementation of GPU memory allocator with move semantics and double-free prevention
 * @version 0.1.0
 * @note Gap Fix: CWE-415 (double-free), CWE-672 (use-after-free)
 */

#include "gpu/gpu_memory_allocator.h"
#include "gpu/gpu_backend_dispatch_contract.h"
#include "gpu/gpu_backend_dispatch_diagnostics.h"
#include <cuda_runtime.h>
#include <algorithm>
#include <utility>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <spdlog/spdlog.h>
#include <chrono>

namespace themis {
namespace gpu {

// =============================================================================
// GPUMemoryAllocator Implementation
// =============================================================================

GPUMemoryAllocator::GPUMemoryAllocator(const Config& config)
    : config_(config),
      next_alloc_id_(1),
      is_moved_from_(false) {
    
    if (config.device_id < 0) {
        throw std::invalid_argument("device_id must be >= 0");
    }

    // Verify device exists
    int device_count;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err != cudaSuccess || config.device_id >= device_count) {
        throw std::runtime_error("Invalid device ID: " + std::string(cudaGetErrorString(err)));
    }

    // Pre-allocate pool if configured
    if (config.pool_size > 0) {
        try {
            void* pool_ptr = nullptr;
            err = cudaMalloc(&pool_ptr, config.pool_size);
            if (err != cudaSuccess) {
                throw std::runtime_error("Failed to allocate memory pool: " + std::string(cudaGetErrorString(err)));
            }
            // Store as allocation for tracking
            MemoryAllocation pool_alloc;
            pool_alloc.device_ptr = pool_ptr;
            pool_alloc.host_ptr = nullptr;
            pool_alloc.size = config.pool_size;
            pool_alloc.device_id = config.device_id;
            pool_alloc.is_unified = false;
            pool_alloc.allocation_id = next_alloc_id_++;
            allocations_.push_back(pool_alloc);
        } catch (...) {
            throw;
        }
    }
}

GPUMemoryAllocator::~GPUMemoryAllocator() noexcept {
    cleanup();
}

GPUMemoryAllocator::GPUMemoryAllocator(GPUMemoryAllocator&& other) noexcept
    : config_(other.config_),
      allocations_(std::move(other.allocations_)),
      next_alloc_id_(other.next_alloc_id_),
      is_moved_from_(false) {
    
    // Mark source as moved-from to prevent double-free
    other.config_.device_id = -1;
    other.allocations_.clear();
    other.is_moved_from_ = true;
}

GPUMemoryAllocator& GPUMemoryAllocator::operator=(GPUMemoryAllocator&& other) noexcept {
    if (this == &other || other.is_moved_from_) {
        return *this;
    }

    // Release current allocations (double-free safe due to moved-from check)
    cleanup();

    // Transfer ownership
    config_ = other.config_;
    allocations_ = std::move(other.allocations_);
    next_alloc_id_ = other.next_alloc_id_;
    is_moved_from_ = false;

    // Mark source as moved-from
    other.config_.device_id = -1;
    other.allocations_.clear();
    other.is_moved_from_ = true;

    return *this;
}

MemoryAllocation GPUMemoryAllocator::allocate([[maybe_unused]] size_t size) {
    uint64_t start_time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    // Phase 2/3 Hardening: Fail-closed early validation
    if (is_moved_from_) {
        GPUBackendDispatchDiagnostics::emitDiagnostic(
            GPUDispatchErrorCode::ALLOC_INVALID_PARAMS,
            config_.device_id,
            "Cannot allocate from moved-from allocator");
        throw std::logic_error("Cannot allocate from moved-from allocator");
    }

    if (size == 0) {
        GPUBackendDispatchDiagnostics::emitDiagnostic(
            GPUDispatchErrorCode::ALLOC_INVALID_PARAMS,
            config_.device_id,
            "Allocation size must be > 0");
        throw std::invalid_argument("Allocation size must be > 0");
    }

    // Phase 2/3 Hardening: Check against configured maximum allocation size
    if (size > config_.max_alloc_size) {
        GPUBackendDispatchDiagnostics::emitDiagnostic(
            GPUDispatchErrorCode::ALLOC_SIZE_EXCEEDS_LIMIT,
            config_.device_id,
            "Requested size=" + std::to_string(size) + 
            " exceeds max_alloc_size=" + std::to_string(config_.max_alloc_size));
        throw std::invalid_argument("Allocation size exceeds limit");
    }

    void* device_ptr = nullptr;
    void* host_ptr = nullptr;

    try {
        // Allocate device memory
        cudaError_t err = cudaMalloc(&device_ptr, size);
        if (err != cudaSuccess) {
            GPUBackendDispatchDiagnostics::emitDiagnostic(
                GPUDispatchErrorCode::ALLOC_DEVICE_FAILURE,
                config_.device_id,
                "cudaMalloc failed: " + std::string(cudaGetErrorString(err)));
            throw std::runtime_error("Device allocation failed: " + std::string(cudaGetErrorString(err)));
        }

        // Allocate pinned host memory for transfers
        err = cudaMallocHost(&host_ptr, size);
        if (err != cudaSuccess) {
            cudaFree(device_ptr);
            GPUBackendDispatchDiagnostics::emitDiagnostic(
                GPUDispatchErrorCode::ALLOC_DEVICE_FAILURE,
                config_.device_id,
                "cudaMallocHost failed: " + std::string(cudaGetErrorString(err)));
            throw std::runtime_error("Host allocation failed: " + std::string(cudaGetErrorString(err)));
        }

        MemoryAllocation alloc;
        alloc.device_ptr = device_ptr;
        alloc.host_ptr = host_ptr;
        alloc.size = size;
        alloc.device_id = config_.device_id;
        alloc.is_unified = (config_.strategy == Strategy::UNIFIED_MEMORY);
        alloc.allocation_id = next_alloc_id_++;

        allocations_.push_back(alloc);
        
        // Verify bounded runtime contract
        uint64_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count() - start_time;
        if (elapsed_us > GPUBackendDispatchContract::MAX_ALLOCATE_LATENCY_US) {
            auto logger = spdlog::get("gpu");
            if (!logger) {
                logger = spdlog::get("default");
            }
            if (logger) {
                logger->warn(
                    "allocate exceeded SLA: elapsed={}µs threshold={}µs size={}",
                    elapsed_us,
                    GPUBackendDispatchContract::MAX_ALLOCATE_LATENCY_US,
                    size);
            }
        }
        
        return alloc;

    } catch (...) {
        if (device_ptr) {
          cudaFree(device_ptr);
        }
        if (host_ptr) {
          cudaFreeHost(host_ptr);
        }
        throw;
    }
}

void GPUMemoryAllocator::deallocate(const MemoryAllocation& alloc) noexcept {
    if (is_moved_from_) {
        return;  // Idempotent: no-op on moved-from allocator
    }

    // Find and remove allocation
    auto it = std::find_if(allocations_.begin(), allocations_.end(),
                          [&alloc](const MemoryAllocation& a) { 
                              return a.allocation_id == alloc.allocation_id; 
                          });

    if (it != allocations_.end()) {
        if (it->device_ptr) {
          cudaFree(it->device_ptr);
        }
        if (it->host_ptr) {
          cudaFreeHost(it->host_ptr);
        }
        allocations_.erase(it);
    }
}

MemoryAllocation GPUMemoryAllocator::reallocate(const MemoryAllocation& alloc, size_t new_size) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot reallocate from moved-from allocator");
    }

    // Allocate new memory
    MemoryAllocation new_alloc = allocate(new_size);

    try {
        // Copy old data to new location
        if (alloc.size > 0) {
            size_t copy_size = std::min(alloc.size, new_size);
            cudaError_t err = cudaMemcpy(new_alloc.device_ptr, alloc.device_ptr, 
                                        copy_size, cudaMemcpyDeviceToDevice);
            if (err != cudaSuccess) {
                deallocate(new_alloc);
                throw std::runtime_error("Reallocation copy failed: " + std::string(cudaGetErrorString(err)));
            }
        }

        // Deallocate old memory
        deallocate(alloc);
        return new_alloc;

    } catch (...) {
        deallocate(new_alloc);
        throw;
    }
}

void GPUMemoryAllocator::copy_to_device(const MemoryAllocation& alloc, 
                                        const void* host_data, size_t size) const {
    if (is_moved_from_) {
        throw std::logic_error("Cannot copy from moved-from allocator");
    }

    if (size > alloc.size) {
        throw std::invalid_argument("Copy size exceeds allocation size");
    }

    cudaError_t err = cudaMemcpy(alloc.device_ptr, host_data, size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        throw std::runtime_error("Copy to device failed: " + std::string(cudaGetErrorString(err)));
    }
}

void GPUMemoryAllocator::copy_from_device(void* host_data, 
                                          const MemoryAllocation& alloc, 
                                          size_t size) const {
    if (is_moved_from_) {
        throw std::logic_error("Cannot copy from moved-from allocator");
    }

    if (size > alloc.size) {
        throw std::invalid_argument("Copy size exceeds allocation size");
    }

    cudaError_t err = cudaMemcpy(host_data, alloc.device_ptr, size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        throw std::runtime_error("Copy from device failed: " + std::string(cudaGetErrorString(err)));
    }
}

bool GPUMemoryAllocator::is_moved_from() const noexcept {
    return is_moved_from_;
}

bool GPUMemoryAllocator::is_initialized() const noexcept {
    return !is_moved_from_ && config_.device_id >= 0;
}

const GPUMemoryAllocator::Config& GPUMemoryAllocator::get_config() const noexcept {
    return config_;
}

size_t GPUMemoryAllocator::available_memory() const {
    if (is_moved_from_) {
        throw std::logic_error("Cannot query moved-from allocator");
    }

    size_t free_bytes, total_bytes;
    cudaError_t err = cudaMemGetInfo(&free_bytes, &total_bytes);
    if (err != cudaSuccess) {
        throw std::runtime_error("Failed to query device memory: " + std::string(cudaGetErrorString(err)));
    }

    return free_bytes;
}

size_t GPUMemoryAllocator::allocated_memory() const noexcept {
    size_t total = 0;
    for (const auto& alloc : allocations_) {
        total += alloc.size;
    }
    return total;
}

size_t GPUMemoryAllocator::allocation_count() const noexcept {
    return allocations_.size();
}

void GPUMemoryAllocator::cleanup() noexcept {
    if (!is_moved_from_) {
        for (auto& alloc : allocations_) {
            if (alloc.device_ptr) {
                cudaFree(alloc.device_ptr);
                alloc.device_ptr = nullptr;
            }
            if (alloc.host_ptr) {
                cudaFreeHost(alloc.host_ptr);
                alloc.host_ptr = nullptr;
            }
        }
    }
    allocations_.clear();
}

// =============================================================================
// DeviceMemoryRegion Implementation
// =============================================================================

DeviceMemoryRegion::DeviceMemoryRegion(GPUMemoryAllocator& allocator, size_t size)
    : allocator_(&allocator), is_moved_from_(false) {
    
    alloc_ = allocator.allocate(size);
}

DeviceMemoryRegion::~DeviceMemoryRegion() noexcept {
    if (!is_moved_from_ && allocator_) {
        allocator_->deallocate(alloc_);
    }
}

DeviceMemoryRegion::DeviceMemoryRegion(DeviceMemoryRegion&& other) noexcept
    : allocator_(other.allocator_),
      alloc_(other.alloc_),
      is_moved_from_(false) {
    
    other.allocator_ = nullptr;
    other.is_moved_from_ = true;
}

DeviceMemoryRegion& DeviceMemoryRegion::operator=(DeviceMemoryRegion&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (!is_moved_from_ && allocator_) {
        allocator_->deallocate(alloc_);
    }

    allocator_ = other.allocator_;
    alloc_ = other.alloc_;
    is_moved_from_ = false;

    other.allocator_ = nullptr;
    other.is_moved_from_ = true;

    return *this;
}

void* DeviceMemoryRegion::device_ptr() noexcept {
    return is_moved_from_ ? nullptr : alloc_.device_ptr;
}

const void* DeviceMemoryRegion::device_ptr() const noexcept {
    return is_moved_from_ ? nullptr : alloc_.device_ptr;
}

void* DeviceMemoryRegion::host_ptr() noexcept {
    return is_moved_from_ ? nullptr : alloc_.host_ptr;
}

size_t DeviceMemoryRegion::size() const noexcept {
    return is_moved_from_ ? 0 : alloc_.size;
}

bool DeviceMemoryRegion::is_valid() const noexcept {
    return !is_moved_from_ && alloc_.device_ptr != nullptr;
}

} // namespace gpu
} // namespace themis
