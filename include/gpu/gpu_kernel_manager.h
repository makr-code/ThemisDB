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

struct CudaKernelHandle;

class GPUKernelManager {
public:
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

    GPUKernelManager() noexcept = default;

    GPUKernelManager(const std::string& kernel_name, 
                     int device_id, 
                     const Config& config);

    ~GPUKernelManager() noexcept;

    // --- Move semantics (enabled) ---

    GPUKernelManager(GPUKernelManager&& other) noexcept;

    GPUKernelManager& operator=(GPUKernelManager&& other) noexcept;

    // --- Copy semantics (deleted) ---
    GPUKernelManager(const GPUKernelManager&) = delete;
    GPUKernelManager& operator=(const GPUKernelManager&) = delete;

    /**
     * @brief --- Kernel execution ---
     * @param[in] args Input parameter.
     */

    void launch(const void* args) const;

    bool wait(uint32_t timeout_ms = 0) const;

    /**
     * @brief Is running.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_running() const noexcept;

    /**
     * @brief --- State validation ---
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */

    bool is_moved_from() const noexcept;

    /**
     * @brief Is valid.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_valid() const noexcept;

    /**
     * @brief Device id.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    int device_id() const noexcept;

    /**
     * @brief Kernel name.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const std::string& kernel_name() const noexcept;

private:
    /**
     * @brief Cleanup.
     * @note Exception safety: noexcept.
     */
    void cleanup() noexcept;

    std::unique_ptr<CudaKernelHandle> handle_;

    Config config_;

    int device_id_;

    std::string kernel_name_;

    bool is_moved_from_;
};

class KernelArgumentBuffer {
public:
    KernelArgumentBuffer(size_t size, int device_id);

    ~KernelArgumentBuffer() noexcept;

    // Move semantics
    KernelArgumentBuffer(KernelArgumentBuffer&& other) noexcept;
    KernelArgumentBuffer& operator=(KernelArgumentBuffer&& other) noexcept;

    // No copy
    KernelArgumentBuffer(const KernelArgumentBuffer&) = delete;
    KernelArgumentBuffer& operator=(const KernelArgumentBuffer&) = delete;

    /**
     * @brief Device ptr.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    void* device_ptr() noexcept;
    /**
     * @brief Device ptr.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    const void* device_ptr() const noexcept;

    /**
     * @brief Host ptr.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    void* host_ptr() noexcept;

    /**
     * @brief Upload.
     */
    void upload() const;

    /**
     * @brief Download.
     */
    void download() const;

    /**
     * @brief Size.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t size() const noexcept;

    /**
     * @brief Is valid.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_valid() const noexcept;

private:
    /**
     * @brief Cleanup.
     * @note Exception safety: noexcept.
     */
    void cleanup() noexcept;

    void* device_ptr_;
    void* host_ptr_;
    size_t size_;
    int device_id_;
    bool is_moved_from_;
};

} // namespace gpu
} // namespace themis
