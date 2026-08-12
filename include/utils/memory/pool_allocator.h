/**
 * @file pool_allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
    
    AllocationStats() = default;
    AllocationStats(const AllocationStats& other) noexcept
        : total_allocations(other.total_allocations.load()),
          total_deallocations(other.total_deallocations.load()),
          allocation_failures(other.allocation_failures.load()),
          bytes_allocated(other.bytes_allocated.load()),
          bytes_freed(other.bytes_freed.load()),
          peak_memory_usage(other.peak_memory_usage.load()) {}
    
    AllocationStats& operator=(const AllocationStats& other) noexcept {
        total_allocations.store(other.total_allocations.load());
        total_deallocations.store(other.total_deallocations.load());
        allocation_failures.store(other.allocation_failures.load());
        bytes_allocated.store(other.bytes_allocated.load());
        bytes_freed.store(other.bytes_freed.load());
        peak_memory_usage.store(other.peak_memory_usage.load());
        return *this;
    }
    
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
    [[nodiscard]] virtual Result<void*> allocate(size_t size, AllocationHint hint = AllocationHint::NONE) = 0;
    
    /**
     * @brief Deallocate memory
     * @param ptr Pointer to memory to free
     * @return Success or error
     */
    [[nodiscard]] virtual Result<void> deallocate(void* ptr) = 0;
    
    /**
     * @brief Get allocator statistics
     */
    [[nodiscard]] virtual const AllocationStats& getStats() const = 0;
    
    /**
     * @brief Reset allocator state (free all allocations)
     * 
     * WARNING: This method is not thread-safe with respect to statistics.
     * It should only be called when no other threads are actively using
     * the allocator. Typically used during shutdown or testing.
     */
    [[nodiscard]] virtual Result<void> reset() = 0;
    
    /**
     * @brief Get allocator name for diagnostics
     */
    [[nodiscard]] virtual const char* getName() const = 0;
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
    /**
     * @brief Release the buddy pool and allocator metadata.
     *
     * Cleanup is RAII-backed and guaranteed not to throw during shutdown.
     */
    ~BuddyAllocator() noexcept override;
    
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
    /**
     * @brief Release all slabs owned by the allocator.
     *
     * Cleanup is RAII-backed and guaranteed not to throw during shutdown.
     */
    ~SlabAllocator() noexcept override;
    
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
        size_t buddy_pool_size;
        size_t buddy_min_block;
        
        // Common slab sizes (powers of 2)
        std::vector<size_t> slab_sizes;
        size_t slab_objects_per_slab;
        size_t slab_max_slabs;
        
        size_t stack_capacity;
        
        bool enable_thread_local_cache;
        bool enable_statistics;
        
        Config()
            : buddy_pool_size(16 * 1024 * 1024),
              buddy_min_block(64),
              slab_sizes({64, 128, 256, 512, 1024, 2048}),
              slab_objects_per_slab(64),
              slab_max_slabs(256),
              stack_capacity(1 * 1024 * 1024),
              enable_thread_local_cache(true),
              enable_statistics(true) {}
    };
    
    PoolAllocator();
    explicit PoolAllocator(const Config& config);
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
