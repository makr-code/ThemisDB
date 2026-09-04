/**
 * @file gpu_memory_pool.cpp
 * @brief GPUMemoryPool implementation
 */

#include "gpu/gpu_memory_pool_safety.h"
#include "gpu/gpu_safe_raii.h"
#include <algorithm>
#include <cstring>

namespace themis {
namespace gpu {

GPUMemoryPool::GPUMemoryPool(const Config& config)
    : config_(config) {
    
    if (config.total_pool_size > 0) {
        // Pre-allocate pool
        try {
            void* pool_ptr = nullptr;
            CUDA_CHECK(cudaMalloc(&pool_ptr, config.total_pool_size));
            
            head_ = new Block{
                pool_ptr,
                config.total_pool_size,
                false,  // Initially free
                nullptr,
                next_allocation_id_++
            };
            total_freed_ = config.total_pool_size;
        } catch (...) {
            head_ = nullptr;
        }
    }
}

GPUMemoryPool::~GPUMemoryPool() noexcept {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    Block* current = head_;
    while (current) {
        Block* next = current->next;
        if (current->ptr) {
            cudaFree(current->ptr);  // No error checking in destructor
        }
        delete current;
        current = next;
    }
    head_ = nullptr;
}

void* GPUMemoryPool::allocate(size_t size) {
    if (size == 0) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(pool_mutex_);

    // Try to find a free block with sufficient size
    Block* current = head_;
    while (current) {
        if (!current->in_use && current->size >= size) {
            // Use this block
            void* result = current->ptr;
            
            // If block is larger than needed, split it
            if (current->size > size) {
                Block* new_block = new Block{
                    static_cast<char*>(current->ptr) + size,
                    current->size - size,
                    false,
                    current->next,
                    next_allocation_id_++
                };
                current->next = new_block;
                current->size = size;
            }

            current->in_use = true;
            total_allocated_ += size;

            // Check if defragmentation needed
            if (getFragmentationRatio() > config_.fragmentation_threshold) {
                coalesceAdjacentBlocks();
            }

            return result;
        }
        current = current->next;
    }

    // No free block found, allocate new memory
    void* new_ptr = nullptr;
    CUDA_CHECK(cudaMalloc(&new_ptr, size));

    Block* new_block = new Block{
        new_ptr,
        size,
        true,
        head_,
        next_allocation_id_++
    };
    head_ = new_block;
    total_allocated_ += size;

    return new_ptr;
}

bool GPUMemoryPool::deallocate(void* ptr) {
    if (!ptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(pool_mutex_);

    Block* block = findBlock(ptr);
    if (!block) {
        return false;
    }

    block->in_use = false;
    total_freed_ += block->size;

    // Try to coalesce with adjacent blocks
    coalesceAdjacentBlocks();

    return true;
}

double GPUMemoryPool::getFragmentationRatio() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    size_t fragmented = computeFragmentedSize();
    if (total_freed_ == 0) {
        return 0.0;
    }
    return static_cast<double>(fragmented) / static_cast<double>(total_freed_);
}

size_t GPUMemoryPool::defragment() {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    size_t initial_blocks = 0;
    Block* current = head_;
    while (current) {
        initial_blocks++;
        current = current->next;
    }

    coalesceAdjacentBlocks();

    size_t final_blocks = 0;
    current = head_;
    while (current) {
        final_blocks++;
        current = current->next;
    }

    return initial_blocks - final_blocks;  // Number of blocks coalesced
}

GPUMemoryPool::Statistics GPUMemoryPool::getStatistics() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    Statistics stats;
    stats.total_allocated = total_allocated_;
    stats.total_freed = total_freed_;
    stats.fragmentation_ratio = getFragmentationRatio();

    Block* current = head_;
    while (current) {
        if (current->in_use) {
            stats.num_allocated_blocks++;
        } else {
            stats.num_free_blocks++;
        }
        current = current->next;
    }

    return stats;
}

size_t GPUMemoryPool::checkForLeaks() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    size_t potential_leaks = 0;
    Block* current = head_;
    while (current) {
        if (current->in_use && current->allocation_id > 0) {
            potential_leaks++;
        }
        current = current->next;
    }
    return potential_leaks;
}

GPUMemoryPool::Block* GPUMemoryPool::findBlock(void* ptr) {
    Block* current = head_;
    while (current) {
        if (current->ptr == ptr) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

void GPUMemoryPool::coalesceAdjacentBlocks() {
    if (!head_) {
      return;
    }

    Block* current = head_;
    while (current && current->next) {
        // Check if current and next are adjacent and both free
        if (!current->in_use && !current->next->in_use &&
            static_cast<char*>(current->ptr) + current->size == current->next->ptr) {
            
            // Coalesce: merge current and next
            Block* next = current->next;
            current->size += next->size;
            current->next = next->next;
            delete next;
            
            // Continue checking current again (may coalesce with new next)
            continue;
        }
        current = current->next;
    }
}

size_t GPUMemoryPool::computeFragmentedSize() const {
    size_t fragmented = 0;
    Block* current = head_;
    
    while (current) {
        if (!current->in_use) {
            // Check if this free block is surrounded by in-use blocks
            bool prev_in_use = (current == head_) || 
                              (current != head_ && current->next && current->next->in_use);
            bool next_in_use = !current->next || current->next->in_use;
            
            if (prev_in_use && next_in_use && current->size > 0) {
                fragmented += current->size;
            }
        }
        current = current->next;
    }
    
    return fragmented;
}

}} // namespace themis::gpu
