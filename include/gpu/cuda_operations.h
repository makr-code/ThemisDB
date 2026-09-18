/**
 * @file cuda_operations.h
 * @brief CUDA operation wrapper with move semantics and use-after-move detection
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 * @note Gap Categories: CWE-672 (use-after-free), CWE-457 (uninitialized variable)
 * 
 * Provides:
 * - Stateful CUDA operation management
 * - Move semantics for device-side operations
 * - Use-after-move prevention with noexcept guarantees
 * - Stream-based asynchronous operation tracking
 * 
 * @see ThemisDB Remediation Roadmap: Sprint 8 Phase 1C
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <stdexcept>

// Test compatibility: provide SKIP() macro alias to GTEST_SKIP() when available
#ifndef SKIP
#ifdef GTEST_SKIP
#define SKIP() GTEST_SKIP()
#endif
#endif

namespace themis {
namespace gpu {

class CudaStream {
public:
    CudaStream(int device_id, int priority = 0);

    ~CudaStream() noexcept;

    // Move semantics
    CudaStream(CudaStream&& other) noexcept;
    CudaStream& operator=(CudaStream&& other) noexcept;

    // No copy
    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;

    /**
     * @brief Get handle.
     * @return Pointer to the result.
     */
    void* get_handle() const;

    /**
     * @brief Synchronize.
     */
    void synchronize() const;

    /**
     * @brief Is ready.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_ready() const noexcept;

    /**
     * @brief Is valid.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_valid() const noexcept;

    /**
     * @brief Is moved from.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_moved_from() const noexcept;

private:
    void* stream_handle_;
    int device_id_;
    bool is_moved_from_;
};

class CudaOperation {
public:
    enum class Status {
        PENDING,       ///< Operation queued but not started
        RUNNING,       ///< Operation currently executing
        COMPLETED,     ///< Operation finished successfully
        FAILED,        ///< Operation failed
        MOVED_FROM,    ///< Moved-from state (operation transferred to another object)
    };

    CudaOperation(const CudaStream& stream, const std::string& name);

    ~CudaOperation() noexcept;

    // Move semantics
    CudaOperation(CudaOperation&& other) noexcept;
    CudaOperation& operator=(CudaOperation&& other) noexcept;

    // No copy
    CudaOperation(const CudaOperation&) = delete;
    CudaOperation& operator=(const CudaOperation&) = delete;

    /**
     * @brief --- Operation lifecycle ---
     */

    void record_event();

    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * @brief Get status.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    Status get_status() const noexcept;

    /**
     * @brief Mark completed.
     * @note Exception safety: noexcept.
     */
    void mark_completed() noexcept;

    /**
     * @brief Mark failed.
     * @param[in] error_msg Input parameter.
     * @note Exception safety: noexcept.
     */
    void mark_failed(const std::string& error_msg) noexcept;

    /**
     * @brief --- Queries ---
     * @return Return value.
     * @note Exception safety: noexcept.
     */

    const std::string& get_name() const noexcept;

    /**
     * @brief Get error.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const std::string& get_error() const noexcept;

    /**
     * @brief Get device id.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    int get_device_id() const noexcept;

    /**
     * @brief Is moved from.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_moved_from() const noexcept;

private:
    void* event_handle_;
    const CudaStream* stream_;
    std::string name_;
    std::string error_msg_;
    Status status_;
    bool is_moved_from_;
};

class CudaOperationBatch {
public:
    /**
     * @brief Cuda Operation Batch.
     * @param[in] stream Input parameter.
     * @return Return value.
     */
    explicit CudaOperationBatch(const CudaStream& stream);

    ~CudaOperationBatch() noexcept;

    // Move semantics
    CudaOperationBatch(CudaOperationBatch&& other) noexcept;
    CudaOperationBatch& operator=(CudaOperationBatch&& other) noexcept;

    // No copy
    CudaOperationBatch(const CudaOperationBatch&) = delete;
    CudaOperationBatch& operator=(const CudaOperationBatch&) = delete;

    /**
     * @brief Add operation.
     * @param[in] op Input parameter.
     */
    void add_operation(CudaOperation&& op);

    bool wait_all(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * @brief Size.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t size() const noexcept;

    /**
     * @brief All completed.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool all_completed() const noexcept;

    /**
     * @brief Failed count.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t failed_count() const noexcept;

    /**
     * @brief Is valid.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_valid() const noexcept;

    /**
     * @brief Is moved from.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_moved_from() const noexcept;

private:
    const CudaStream* stream_;
    std::vector<CudaOperation> operations_;
    bool is_moved_from_;
};

} // namespace gpu
} // namespace themis
