/**
 * @file gpu_memory_pool.h
 * @brief GPU Memory Pool Safety — Fragmentation Control and Leak Prevention
 *
 * @version v1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Wave A Batch A-8 Implementation (Update existing)
 *
 * Provides memory pool management with:
 * - Block tracking (allocation metadata)
 * - Coalescing of adjacent free blocks (fragmentation control)
 * - Defragmentation when fragmentation exceeds threshold
 * - Thread-safe allocation/deallocation
 *
 * ## Fragmentation Control
 * - Target: < 10% fragmentation
 * - Monitors: (fragmented_free_size / total_free_size)
 * - Triggers: Defragmentation when ratio > 10%
 *
 * @error 7800: Memory pool allocation failed
 * @error 7801: Memory pool deallocation failed
 * @error 7802: Memory leak detected
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <memory>
#include <mutex>

namespace themis {
namespace gpu {

/**
 * @class GPUMemoryPool
 * @brief GPU memory pool with fragmentation control and leak prevention
 */
class GPUMemoryPool {
public:
    /// @brief Memory pool configuration
    struct Config {
        size_t total_pool_size = 0;        ///< Pre-allocate pool (0 = no pre-alloc)
        double fragmentation_threshold = 0.10;  ///< Trigger defrag at 10%
        bool enable_statistics = true;     ///< Track allocation statistics
    };

    /// @brief Construct memory pool
    /// @param config Pool configuration
    explicit GPUMemoryPool(const Config& config = {});

    /// @brief Destructor — frees all allocations
    ~GPUMemoryPool() noexcept;

    /// Delete copy operations
    GPUMemoryPool(const GPUMemoryPool&) = delete;
    GPUMemoryPool& operator=(const GPUMemoryPool&) = delete;

    /// Allow move operations
    GPUMemoryPool(GPUMemoryPool&&) noexcept = default;
    GPUMemoryPool& operator=(GPUMemoryPool&&) noexcept = default;

    /// @brief Allocate memory from pool
    /// @param size Number of bytes to allocate
    /// @return Device pointer if successful; nullptr if failed
    /// @throws std::runtime_error on allocation failure
    void* allocate(size_t size);

    /// @brief Deallocate memory back to pool
    /// @param ptr Device pointer to deallocate
    /// @return true if deallocation succeeded; false if pointer not found
    bool deallocate(void* ptr);

    /// @brief Get fragmentation ratio
    /// @return Ratio of fragmented free space to total free space (0.0-1.0)
    double getFragmentationRatio() const;

    /// @brief Force defragmentation of pool
    /// @return Number of blocks coalesced
    size_t defragment();

    /// @brief Get current statistics
    /// @return Structure with pool metrics
    struct Statistics {
        size_t total_allocated = 0;      ///< Total bytes allocated
        size_t total_freed = 0;          ///< Total bytes freed/available
        size_t num_allocated_blocks = 0; ///< Number of in-use blocks
        size_t num_free_blocks = 0;      ///< Number of free blocks
        double fragmentation_ratio = 0.0;
    };

    /// @brief Get pool statistics
    /// @return Current statistics
    Statistics getStatistics() const;

    /// @brief Check for memory leaks
    /// @return Number of potentially leaked blocks
    size_t checkForLeaks() const;

private:
    struct Block {
        void* ptr;
        size_t size;
        bool in_use;
        Block* next;
        int64_t allocation_id;
    };

    Config config_;
    mutable std::mutex pool_mutex_;
    Block* head_ = nullptr;
    size_t total_allocated_ = 0;
    size_t total_freed_ = 0;
    int64_t next_allocation_id_ = 1;

    Block* findBlock(void* ptr);
    void coalesceAdjacentBlocks();
    size_t computeFragmentedSize() const;
};

}} // namespace themis::gpu
