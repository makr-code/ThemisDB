/**
 * @file unified_memory_coordinator.cpp
 * @brief UnifiedMemoryBuffer implementation
 */

#include "gpu/unified_memory_coordinator.h"
#include <cuda_runtime.h>

namespace themis {
namespace gpu {

UnifiedMemoryBuffer::UnifiedMemoryBuffer(size_t size)
    : ptr_(nullptr), size_(size), owner_(Owner::UNOWNED), conflict_(false) {
    
    if (size > 0) {
        ptr_ = allocateUnifiedMemory(size);
    }
}

UnifiedMemoryBuffer::~UnifiedMemoryBuffer() noexcept {
    if (ptr_) {
        freeUnifiedMemory(ptr_);
    }
}

bool UnifiedMemoryBuffer::acquireForCPU() {
    if (!isValid()) {
        return false;
    }

    Owner expected = Owner::UNOWNED;
    
    // Try to acquire if currently unowned
    if (owner_.compare_exchange_strong(expected, Owner::CPU, 
                                       std::memory_order_acquire,
                                       std::memory_order_acquire)) {
        conflict_.store(false);
        return true;
    }

    // Check if GPU currently owns
    if (expected == Owner::GPU) {
        // GPU has access - must synchronize
        try {
            CUDA_CHECK(cudaDeviceSynchronize());
            owner_.store(Owner::CPU, std::memory_order_release);
            conflict_.store(false);
            return true;
        } catch (const std::exception&) {
            conflict_.store(true);
            return false;
        }
    }

    // CPU already owns or other conflict
    conflict_.store(true);
    return false;
}

bool UnifiedMemoryBuffer::acquireForGPU() {
    if (!isValid()) {
        return false;
    }

    Owner expected = Owner::UNOWNED;
    
    // Try to acquire if currently unowned
    if (owner_.compare_exchange_strong(expected, Owner::GPU,
                                       std::memory_order_acquire,
                                       std::memory_order_acquire)) {
        conflict_.store(false);
        return true;
    }

    // Check if CPU currently owns
    if (expected == Owner::CPU) {
        // CPU has access - just transfer ownership
        // (CUDA handles coherence for unified memory)
        owner_.store(Owner::GPU, std::memory_order_release);
        conflict_.store(false);
        return true;
    }

    // GPU already owns or other conflict
    conflict_.store(true);
    return false;
}

bool UnifiedMemoryBuffer::releaseOwnership() {
    Owner current = owner_.load(std::memory_order_acquire);
    
    if (current == Owner::UNOWNED) {
        return false;  // Already unowned
    }

    owner_.store(Owner::UNOWNED, std::memory_order_release);
    return true;
}

UnifiedMemoryBuffer::Owner UnifiedMemoryBuffer::getCurrentOwner() const noexcept {
    return owner_.load(std::memory_order_acquire);
}

void* UnifiedMemoryBuffer::get() noexcept {
    return ptr_;
}

const void* UnifiedMemoryBuffer::get() const noexcept {
    return ptr_;
}

size_t UnifiedMemoryBuffer::size() const noexcept {
    return size_;
}

bool UnifiedMemoryBuffer::isValid() const noexcept {
    return ptr_ != nullptr && size_ > 0;
}

void UnifiedMemoryBuffer::synchronize() {
    if (!isValid()) {
        throw std::runtime_error("UnifiedMemoryBuffer: Buffer not valid");
    }

    CUDA_CHECK(cudaDeviceSynchronize());
}

bool UnifiedMemoryBuffer::hadConflict() const noexcept {
    return conflict_.load(std::memory_order_acquire);
}

void* UnifiedMemoryBuffer::allocateUnifiedMemory(size_t size) {
    void* ptr = nullptr;
    CUDA_CHECK(cudaMallocManaged(&ptr, size));
    return ptr;
}

void UnifiedMemoryBuffer::freeUnifiedMemory(void* ptr) noexcept {
    if (ptr) {
        cudaFree(ptr);  // No error checking in destructor
    }
}

}} // namespace themis::gpu
