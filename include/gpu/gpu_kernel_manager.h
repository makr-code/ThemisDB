/**
 * @file gpu_kernel_manager.h
 * @brief GPU kernel execution manager with move semantics support
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 * @note Gap Categories: CWE-457 (uninitialized variable), CWE-415 (double-free), CWE-672 (use-after-free)
 * 
 * Provides:
 * - RAII-based GPU kernel resource management
 * - Move constructors/assignment operators with noexcept guarantees
 * - Proper moved-from state validation
 * - GPU resource cleanup via std::unique_ptr
 * 
 * @see ThemisDB Remediation Roadmap: Sprint 8 Phase 1C
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

namespace themis {
namespace gpu {

/**
 * @brief Forward declaration of CUDA kernel handle
 */
struct CudaKernelHandle;

/**
 * @brief GPU kernel execution context with move semantics
 * 
 * Thread-safe representation of a kernel execution task with:
 * - Automatic resource cleanup via RAII
 * - Move-only semantics (no copying)
 * - Moved-from state validation
 * - Noexcept move operations
 */
class GPUKernelManager {
public:
    /**
     * @brief Kernel execution configuration
     */
    struct Config {
        uint32_t block_size_x = 256;
        uint32_t block_size_y = 1;
        uint32_t block_size_z = 1;
        uint32_t grid_size_x = 1;
        uint32_t grid_size_y = 1;
        uint32_t grid_size_z = 1;
        size_t shared_memory_bytes = 0;
        int priority = 0;
    };

    /**
     * @brief Default constructor - creates uninitialized manager
     * 
     * Creates a manager in moved-from state (safe for destruction/reassignment).
     * Must not be used for kernel submission without first assigning from
     * another initialized manager.
     */
    GPUKernelManager() noexcept = default;

    /**
     * @brief Constructor with kernel initialization
     * 
     * @param kernel_name Name of the kernel function to manage
     * @param device_id GPU device ID (0-based)
     * @param config Kernel execution configuration
     * @throws std::runtime_error If kernel registration fails
     * @throws std::invalid_argument If device_id is invalid
     * 
     * @warning GPU resources are allocated in VRAM. Ensure sufficient
     *          device memory before construction.
     */
    GPUKernelManager(const std::string& kernel_name, 
                     int device_id, 
                     const Config& config);

    /**
     * @brief Destructor - releases GPU resources
     * 
     * Safe to call on:
     * - Fully initialized managers (releases resources)
     * - Moved-from managers (no-op)
     * - Default-constructed managers (no-op)
     */
    ~GPUKernelManager() noexcept;

    // --- Move semantics (enabled) ---

    /**
     * @brief Move constructor
     * 
     * @param other Manager to move from - becomes moved-from state after call
     * 
     * Transfer of ownership guarantees:
     * - All GPU resources transferred to this manager
     * - `other` becomes safe moved-from state
     * - No-throw guarantee: does not allocate
     * 
     * @post other.is_moved_from() == true
     * @post is_valid() depends on other's validity
     */
    GPUKernelManager(GPUKernelManager&& other) noexcept;

    /**
     * @brief Move assignment operator
     * 
     * @param other Manager to move from - becomes moved-from state after call
     * @return Reference to this manager
     * 
     * Release-and-acquire pattern:
     * - Releases current resources if this is valid
     * - Acquires `other`'s resources
     * - `other` becomes moved-from state
     * - Self-assignment safe via moved-from check
     * 
     * @post other.is_moved_from() == true
     * 
     * @note Noexcept: resource cleanup is best-effort; exceptions from
     *       cleanup are logged but not thrown
     */
    GPUKernelManager& operator=(GPUKernelManager&& other) noexcept;

    // --- Copy semantics (deleted) ---
    GPUKernelManager(const GPUKernelManager&) = delete;
    GPUKernelManager& operator=(const GPUKernelManager&) = delete;

    // --- Kernel execution ---

    /**
     * @brief Submit kernel for execution on GPU
     * 
     * @param args Kernel arguments (opaque - type-checked by CUDA)
     * @throws std::runtime_error If kernel submission fails
     * @throws std::logic_error If called on moved-from manager
     * 
     * @pre !is_moved_from()
     * @pre is_valid()
     */
    void launch(const void* args) const;

    /**
     * @brief Wait for current kernel execution to complete
     * 
     * @param timeout_ms Timeout in milliseconds (0 = infinite)
     * @return true if kernel completed, false if timeout
     * @throws std::runtime_error If synchronization fails
     * @throws std::logic_error If called on moved-from manager
     * 
     * @pre !is_moved_from()
     */
    bool wait(uint32_t timeout_ms = 0) const;

    /**
     * @brief Query kernel execution status
     * 
     * @return true if kernel is currently running, false if idle or error
     * @throws std::runtime_error If query fails
     * 
     * Safe to call on moved-from managers (returns false).
     */
    bool is_running() const noexcept;

    // --- State validation ---

    /**
     * @brief Check if manager is in moved-from state
     * 
     * @return true if this manager has been moved-from
     * 
     * Moved-from managers are:
     * - Safe to destroy
     * - Safe to reassign
     * - NOT safe to use for kernel operations
     * 
     * @post Calling any operation on a moved-from manager that requires
     *       validity will throw std::logic_error
     */
    bool is_moved_from() const noexcept;

    /**
     * @brief Check if manager holds valid GPU resources
     * 
     * @return true if manager is initialized with GPU resources
     * 
     * Invalid managers may be:
     * - Default-constructed (not yet initialized)
     * - Moved-from (resources transferred)
     * - Failed during initialization
     */
    bool is_valid() const noexcept;

    /**
     * @brief Get GPU device ID associated with this kernel
     * 
     * @return Device ID, or -1 if manager is invalid/moved-from
     */
    int device_id() const noexcept;

    /**
     * @brief Get kernel name
     * 
     * @return Name of managed kernel, or empty string if moved-from
     */
    const std::string& kernel_name() const noexcept;

private:
    /**
     * @brief Cleanup GPU resources - safe to call multiple times
     * 
     * Called by:
     * - Destructor
     * - Move assignment (to release old resources)
     * 
     * Idempotent and noexcept: multiple calls are safe.
     */
    void cleanup() noexcept;

    /// Opaque GPU kernel handle (null if moved-from or uninitialized)
    std::unique_ptr<CudaKernelHandle> handle_;

    /// Cached kernel configuration
    Config config_;

    /// Device ID (-1 if invalid)
    int device_id_;

    /// Kernel name (empty if moved-from)
    std::string kernel_name_;

    /// Moved-from state flag
    bool is_moved_from_;
};

/**
 * @brief RAII wrapper for kernel argument buffers
 * 
 * Automatically allocates GPU memory for kernel arguments and
 * releases on destruction or move.
 */
class KernelArgumentBuffer {
public:
    /**
     * @brief Allocate GPU argument buffer
     * 
     * @param size Size of buffer in bytes
     * @param device_id GPU device ID
     * @throws std::runtime_error If allocation fails
     * @throws std::invalid_argument If size is 0
     */
    KernelArgumentBuffer(size_t size, int device_id);

    /**
     * @brief Destructor - releases GPU memory
     */
    ~KernelArgumentBuffer() noexcept;

    // Move semantics
    KernelArgumentBuffer(KernelArgumentBuffer&& other) noexcept;
    KernelArgumentBuffer& operator=(KernelArgumentBuffer&& other) noexcept;

    // No copy
    KernelArgumentBuffer(const KernelArgumentBuffer&) = delete;
    KernelArgumentBuffer& operator=(const KernelArgumentBuffer&) = delete;

    /**
     * @brief Get GPU pointer to argument buffer
     * 
     * @return Device pointer (void*), null if moved-from
     */
    void* device_ptr() noexcept;
    const void* device_ptr() const noexcept;

    /**
     * @brief Get CPU-side copy of arguments
     * 
     * @return Host pointer to argument data
     */
    void* host_ptr() noexcept;

    /**
     * @brief Copy arguments from host to device
     * 
     * @throws std::runtime_error If copy fails
     * @throws std::logic_error If moved-from
     */
    void upload() const;

    /**
     * @brief Copy arguments from device to host
     * 
     * @throws std::runtime_error If copy fails
     * @throws std::logic_error If moved-from
     */
    void download() const;

    /**
     * @brief Get buffer size in bytes
     * 
     * @return Size, or 0 if moved-from
     */
    size_t size() const noexcept;

    /**
     * @brief Check if buffer is valid (not moved-from)
     * 
     * @return true if buffer holds GPU memory
     */
    bool is_valid() const noexcept;

private:
    void cleanup() noexcept;

    void* device_ptr_;
    void* host_ptr_;
    size_t size_;
    int device_id_;
    bool is_moved_from_;
};

} // namespace gpu
} // namespace themis
