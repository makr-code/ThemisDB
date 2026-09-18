/**
 * @file unified_memory_coordinator.h
 * @brief Unified Memory CPU/GPU Coordination — Ownership Tracking and Synchronization
 *
 * @version v1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Wave A Batch A-8 Implementation
 *
 * Manages CUDA unified memory with explicit CPU/GPU ownership tracking to prevent:
 * - Concurrent CPU/GPU access (data corruption)
 * - Page thrashing (performance degradation)
 * - Use-after-free (ownership validation)
 * - Synchronization violations (missing cudaDeviceSynchronize)
 *
 * ## Ownership Model
 * ```
 * UNOWNED
 *   ↓
 * CPU (exclusive access from CPU)
 * GPU (exclusive access from GPU kernel)
 * 
 * Transitions:
 * CPU → GPU: CPU releases, GPU acquires (no sync needed for unified memory)
 * GPU → CPU: GPU completes, CPU acquires (cudaDeviceSynchronize required)
 * ```
 *
 * ## Usage Example
 * ```cpp
 * UnifiedMemoryBuffer buffer(1024 * sizeof(float));
 *
 * // CPU usage
 * {
 *     auto access = buffer.acquireForCPU();
 *     memcpy(buffer.get(), host_data, 1024 * sizeof(float));
 * }  // CPU access released
 *
 * // GPU usage
 * {
 *     auto access = buffer.acquireForGPU();
 *     kernel<<<blocks, threads>>>(buffer.get());
 *     CUDA_CHECK(cudaDeviceSynchronize());
 * }  // GPU access released
 * ```
 *
 * @error 7600: Unified memory allocation failed
 * @error 7601: Buffer ownership conflict (concurrent access attempted)
 * @error 7602: Synchronization failed
 */

#pragma once

#include "gpu_safe_raii.h"
#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>

namespace themis {
namespace gpu {

class UnifiedMemoryBuffer {
public:
    enum class Owner {
        UNOWNED = 0,  ///< Not currently owned
        CPU = 1,      ///< CPU has exclusive access
        GPU = 2       ///< GPU has exclusive access
    };

    /**
     * @brief Unified Memory Buffer.
     * @param[in] size Input parameter.
     * @return Return value.
     */
    explicit UnifiedMemoryBuffer(size_t size);

    ~UnifiedMemoryBuffer() noexcept;

    // Delete copy operations
    UnifiedMemoryBuffer(const UnifiedMemoryBuffer&) = delete;
    UnifiedMemoryBuffer& operator=(const UnifiedMemoryBuffer&) = delete;

    // Allow move operations
    UnifiedMemoryBuffer(UnifiedMemoryBuffer&&) noexcept = default;
    UnifiedMemoryBuffer& operator=(UnifiedMemoryBuffer&&) noexcept = default;

    /**
     * @brief Acquire For CPU.
     * @return True when the operation succeeds.
     */
    bool acquireForCPU();

    /**
     * @brief Acquire For GPU.
     * @return True when the operation succeeds.
     */
    bool acquireForGPU();

    /**
     * @brief Release Ownership.
     * @return True when the operation succeeds.
     */
    bool releaseOwnership();

    /**
     * @brief Get Current Owner.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    Owner getCurrentOwner() const noexcept;

    /**
     * @brief Get.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    void* get() noexcept;

    /**
     * @brief Get.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    const void* get() const noexcept;

    /**
     * @brief Size.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t size() const noexcept;

    /**
     * @brief Is Valid.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isValid() const noexcept;

    /**
     * @brief Synchronize.
     */
    void synchronize();

    /**
     * @brief Had Conflict.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool hadConflict() const noexcept;

private:
    void* ptr_;
    size_t size_ = {};
    std::atomic<Owner> owner_;
    std::atomic<bool> conflict_;  // Track if conflict occurred

    /**
     * @brief Allocate Unified Memory.
     * @param[in] size Input parameter.
     * @return Pointer to the result.
     */
    void* allocateUnifiedMemory(size_t size);
    /**
     * @brief Free Unified Memory.
     * @param[in,out] ptr Input/output parameter.
     * @note Exception safety: noexcept.
     */
    void freeUnifiedMemory(void* ptr) noexcept;
};

}} // namespace themis::gpu
