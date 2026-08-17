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

/**
 * @class UnifiedMemoryBuffer
 * @brief Thread-safe unified memory buffer with CPU/GPU ownership tracking
 *
 * Manages CUDA unified memory allocation with atomic ownership state to ensure
 * exclusive access from either CPU or GPU at any given time.
 */
class UnifiedMemoryBuffer {
public:
    /// @brief Owner state of buffer
    enum class Owner {
        UNOWNED = 0,  ///< Not currently owned
        CPU = 1,      ///< CPU has exclusive access
        GPU = 2       ///< GPU has exclusive access
    };

    /// @brief Allocate unified memory buffer
    /// @param size Number of bytes to allocate
    /// @throws std::runtime_error if allocation fails
    explicit UnifiedMemoryBuffer(size_t size);

    /// @brief Destructor — frees unified memory
    ~UnifiedMemoryBuffer() noexcept;

    // Delete copy operations
    UnifiedMemoryBuffer(const UnifiedMemoryBuffer&) = delete;
    UnifiedMemoryBuffer& operator=(const UnifiedMemoryBuffer&) = delete;

    // Allow move operations
    UnifiedMemoryBuffer(UnifiedMemoryBuffer&&) noexcept = default;
    UnifiedMemoryBuffer& operator=(UnifiedMemoryBuffer&&) noexcept = default;

    /// @brief Acquire buffer for CPU access (exclusive)
    /// @return true if acquisition succeeded; false if ownership conflict
    /// @throws std::runtime_error if synchronization fails
    /// @note Blocks until any GPU access completes (via cudaDeviceSynchronize)
    bool acquireForCPU();

    /// @brief Acquire buffer for GPU access (exclusive)
    /// @return true if acquisition succeeded; false if ownership conflict
    /// @note GPU immediately assumes ownership (coherence handled by CUDA)
    bool acquireForGPU();

    /// @brief Release buffer ownership
    /// @return true if released; false if not owned
    /// @note Safe to call even if not owned (no-op)
    bool releaseOwnership();

    /// @brief Get current owner
    /// @return Current owner (CPU, GPU, or UNOWNED)
    Owner getCurrentOwner() const noexcept;

    /// @brief Get buffer pointer
    /// @return Raw pointer to unified memory
    void* get() noexcept;

    /// @brief Get const buffer pointer
    /// @return Const pointer to unified memory
    const void* get() const noexcept;

    /// @brief Get buffer size
    /// @return Size in bytes
    size_t size() const noexcept;

    /// @brief Check if buffer is valid
    /// @return true if allocated and valid
    bool isValid() const noexcept;

    /// @brief Explicit synchronization point
    /// @throws std::runtime_error if cudaDeviceSynchronize fails
    void synchronize();

    /// @brief Check for ownership conflict
    /// @return true if last operation would have conflicted
    bool hadConflict() const noexcept;

private:
    void* ptr_;
    size_t size_;
    std::atomic<Owner> owner_;
    std::atomic<bool> conflict_;  // Track if conflict occurred

    void* allocateUnifiedMemory(size_t size);
    void freeUnifiedMemory(void* ptr) noexcept;
};

}} // namespace themis::gpu
