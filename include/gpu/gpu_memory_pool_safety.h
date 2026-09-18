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

class GPUMemoryPool {
public:
    struct Config {
        size_t total_pool_size = 0;        ///< Pre-allocate pool (0 = no pre-alloc)
        double fragmentation_threshold = 0.10;  ///< Trigger defrag at 10%
        bool enable_statistics = true;     ///< Track allocation statistics
    };

    explicit GPUMemoryPool(const Config& config = {});

    ~GPUMemoryPool() noexcept;

    GPUMemoryPool(const GPUMemoryPool&) = delete;
    GPUMemoryPool& operator=(const GPUMemoryPool&) = delete;

    GPUMemoryPool(GPUMemoryPool&&) noexcept = default;
    GPUMemoryPool& operator=(GPUMemoryPool&&) noexcept = default;

    /**
     * @brief Allocate.
     * @param[in] size Input parameter.
     * @return Pointer to the result.
     */
    void* allocate(size_t size);

    /**
     * @brief Deallocate.
     * @param[in,out] ptr Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool deallocate(void* ptr);

    /**
     * @brief Get Fragmentation Ratio.
     * @return Return value.
     */
    double getFragmentationRatio() const;

    /**
     * @brief Defragment.
     * @return Return value.
     */
    size_t defragment();

    struct Statistics {
        size_t total_allocated = 0;      ///< Total bytes allocated
        size_t total_freed = 0;          ///< Total bytes freed/available
        size_t num_allocated_blocks = 0; ///< Number of in-use blocks
        size_t num_free_blocks = 0;      ///< Number of free blocks
        double fragmentation_ratio = 0.0;
    };

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;

    /**
     * @brief Check For Leaks.
     * @return Return value.
     */
    size_t checkForLeaks() const;

private:
    struct Block {
        void* ptr;
        size_t size = {};
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

    /**
     * @brief Find Block.
     * @param[in,out] ptr Input/output parameter.
     * @return Pointer to the result.
     */
    Block* findBlock(void* ptr);
    /**
     * @brief Coalesce Adjacent Blocks.
     */
    void coalesceAdjacentBlocks();
    /**
     * @brief Compute Fragmented Size.
     * @return Return value.
     */
    size_t computeFragmentedSize() const;
};

}} // namespace themis::gpu
