// ThemisDB Memory Pool Allocator
// High-performance memory management with pooling strategies
//
// Implements multiple allocation strategies:
// - Buddy Allocator: Variable size allocation with low fragmentation
// - Slab Allocator: Fixed-size objects with O(1) operations
// - Stack Allocator: LIFO allocation for temporary objects
//
// Features:
// - Thread-local caches for lock-free single-threaded access
// - Atomic operations for multi-threaded pools
// - CPU cache line alignment support
// - Pool statistics and monitoring
// - Dynamic pool expansion/contraction

#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include "utils/expected.h"

namespace themis {
namespace memory {

// Forward declarations
class BuddyAllocator;
class SlabAllocator;
class StackAllocator;

/**
 * @brief Cache line size for alignment (typically 64 bytes)
 */
constexpr size_t CACHE_LINE_SIZE = 64;

/**
 * @brief Allocation statistics for monitoring
 */
struct AllocationStats {
    std::atomic<uint64_t> total_allocations{0};
    std::atomic<uint64_t> total_deallocations{0};
    std::atomic<uint64_t> allocation_failures{0};
    std::atomic<uint64_t> bytes_allocated{0};
    std::atomic<uint64_t> bytes_freed{0};
    std::atomic<uint64_t> peak_memory_usage{0};
    
    void reset() {
        total_allocations.store(0);
        total_deallocations.store(0);
        allocation_failures.store(0);
        bytes_allocated.store(0);
        bytes_freed.store(0);
        peak_memory_usage.store(0);
    }
    
    uint64_t getCurrentUsage() const {
        return bytes_allocated.load() - bytes_freed.load();
    }
    
    double getSuccessRate() const {
        uint64_t total = total_allocations.load();
        if (total == 0) return 1.0;
        uint64_t failures = allocation_failures.load();
        return 1.0 - (static_cast<double>(failures) / total);
    }
};

/**
 * @brief Allocation hint for optimization
 */
enum class AllocationHint {
    NONE = 0,           // No specific hint
    SHORT_LIVED,        // Object will be freed soon
    LONG_LIVED,         // Object will live for a long time
    CACHE_LINE_ALIGNED, // Align to cache line boundary
    PAGE_ALIGNED        // Align to page boundary (4KB)
};

/**
 * @brief Base interface for memory allocators
 */
class IAllocator {
public:
    virtual ~IAllocator() = default;
    
    /**
     * @brief Allocate memory
     * @param size Number of bytes to allocate
     * @param hint Optional allocation hint for optimization
     * @return Pointer to allocated memory or error
     */
    virtual Result<void*> allocate(size_t size, AllocationHint hint = AllocationHint::NONE) = 0;
    
    /**
     * @brief Deallocate memory
     * @param ptr Pointer to memory to free
     * @return Success or error
     */
    virtual Result<void> deallocate(void* ptr) = 0;
    
    /**
     * @brief Get allocator statistics
     */
    virtual const AllocationStats& getStats() const = 0;
    
    /**
     * @brief Reset allocator state (free all allocations)
     */
    virtual Result<void> reset() = 0;
    
    /**
     * @brief Get allocator name for diagnostics
     */
    virtual const char* getName() const = 0;
};

/**
 * @brief Buddy Allocator - Variable size allocation with binary subdivision
 * 
 * Features:
 * - Allocation: O(log n)
 * - Deallocation: O(log n) with coalescing
 * - Low to medium fragmentation
 * - Best for: Mixed size allocations
 */
class BuddyAllocator : public IAllocator {
public:
    /**
     * @brief Construct buddy allocator
     * @param total_size Total pool size (must be power of 2)
     * @param min_block_size Minimum allocation size (must be power of 2)
     */
    explicit BuddyAllocator(size_t total_size, size_t min_block_size = 64);
    ~BuddyAllocator() override;
    
    Result<void*> allocate(size_t size, AllocationHint hint = AllocationHint::NONE) override;
    Result<void> deallocate(void* ptr) override;
    const AllocationStats& getStats() const override { return stats_; }
    Result<void> reset() override;
    const char* getName() const override { return "BuddyAllocator"; }
    
    /**
     * @brief Get fragmentation ratio (0.0 = no fragmentation, 1.0 = fully fragmented)
     */
    double getFragmentation() const;
    
private:
    struct Block;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    AllocationStats stats_;
};

/**
 * @brief Slab Allocator - Fixed-size object allocation
 * 
 * Features:
 * - Allocation: O(1)
 * - Deallocation: O(1)
 * - Minimal fragmentation
 * - Best for: Object pools (strings, query contexts, etc.)
 */
class SlabAllocator : public IAllocator {
public:
    /**
     * @brief Construct slab allocator
     * @param object_size Size of each object
     * @param objects_per_slab Number of objects per slab
     * @param max_slabs Maximum number of slabs (0 = unlimited)
     */
    explicit SlabAllocator(size_t object_size, size_t objects_per_slab = 64, 
                          size_t max_slabs = 0);
    ~SlabAllocator() override;
    
    Result<void*> allocate(size_t size, AllocationHint hint = AllocationHint::NONE) override;
    Result<void> deallocate(void* ptr) override;
    const AllocationStats& getStats() const override { return stats_; }
    Result<void> reset() override;
    const char* getName() const override { return "SlabAllocator"; }
    
    /**
     * @brief Get number of active slabs
     */
    size_t getSlabCount() const;
    
    /**
     * @brief Get utilization ratio (0.0 = empty, 1.0 = full)
     */
    double getUtilization() const;
    
private:
    struct Slab;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    AllocationStats stats_;
};

/**
 * @brief Stack Allocator - LIFO allocation pattern
 * 
 * Features:
 * - Allocation: O(1)
 * - Deallocation: O(1) (must be LIFO)
 * - Zero fragmentation (if LIFO respected)
 * - Best for: Temporary allocations in scope
 */
class StackAllocator : public IAllocator {
public:
    /**
     * @brief Construct stack allocator
     * @param capacity Maximum stack size in bytes
     */
    explicit StackAllocator(size_t capacity);
    ~StackAllocator() override;
    
    Result<void*> allocate(size_t size, AllocationHint hint = AllocationHint::NONE) override;
    Result<void> deallocate(void* ptr) override;
    const AllocationStats& getStats() const override { return stats_; }
    Result<void> reset() override;
    const char* getName() const override { return "StackAllocator"; }
    
    /**
     * @brief Get current stack position
     */
    size_t getCurrentOffset() const;
    
    /**
     * @brief Get available space
     */
    size_t getAvailableSpace() const;
    
    /**
     * @brief Save current position (for nested scopes)
     */
    size_t savePosition() const;
    
    /**
     * @brief Restore to saved position (fast batch deallocation)
     */
    Result<void> restorePosition(size_t position);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    AllocationStats stats_;
};

/**
 * @brief Pool allocator with multiple allocation strategies
 * 
 * Automatically selects the best allocator based on allocation pattern:
 * - Small, fixed-size allocations -> Slab
 * - Temporary allocations -> Stack
 * - Mixed sizes -> Buddy
 */
class PoolAllocator {
public:
    /**
     * @brief Configuration for pool allocator
     */
    struct Config {
        size_t buddy_pool_size = 16 * 1024 * 1024;  // 16 MB
        size_t buddy_min_block = 64;
        
        // Common slab sizes (powers of 2)
        std::vector<size_t> slab_sizes = {64, 128, 256, 512, 1024, 2048};
        size_t slab_objects_per_slab = 64;
        size_t slab_max_slabs = 256;
        
        size_t stack_capacity = 1 * 1024 * 1024;  // 1 MB
        
        bool enable_thread_local_cache = true;
        bool enable_statistics = true;
    };
    
    explicit PoolAllocator(const Config& config = Config{});
    ~PoolAllocator();
    
    /**
     * @brief Allocate memory using best strategy
     */
    Result<void*> allocate(size_t size, AllocationHint hint = AllocationHint::NONE);
    
    /**
     * @brief Deallocate memory
     */
    Result<void> deallocate(void* ptr);
    
    /**
     * @brief Get combined statistics
     */
    AllocationStats getCombinedStats() const;
    
    /**
     * @brief Get statistics by allocator type
     */
    const AllocationStats& getBuddyStats() const;
    const AllocationStats& getSlabStats(size_t size) const;
    const AllocationStats& getStackStats() const;
    
    /**
     * @brief Reset all allocators
     */
    Result<void> reset();
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace memory
} // namespace themis
