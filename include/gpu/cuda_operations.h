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

/**
 * @brief CUDA stream wrapper with RAII semantics
 * 
 * Manages CUDA stream lifecycle and prevents use-after-move
 * through explicit state tracking.
 */
class CudaStream {
public:
    /**
     * @brief Create CUDA stream on specified device
     * 
     * @param device_id GPU device ID
     * @param priority Stream priority (higher = higher priority)
     * @throws std::runtime_error If stream creation fails
     * @throws std::invalid_argument If device_id is invalid
     */
    CudaStream(int device_id, int priority = 0);

    /**
     * @brief Destructor - destroys CUDA stream
     * 
     * Safe on moved-from streams (no double-destroy).
     */
    ~CudaStream() noexcept;

    // Move semantics
    CudaStream(CudaStream&& other) noexcept;
    CudaStream& operator=(CudaStream&& other) noexcept;

    // No copy
    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;

    /**
     * @brief Get CUDA stream handle
     * 
     * @return Stream handle, or nullptr if moved-from
     * @throws std::logic_error If called on moved-from stream
     */
    void* get_handle() const;

    /**
     * @brief Synchronize on this stream (wait for completion)
     * 
     * @throws std::runtime_error If synchronization fails
     * @throws std::logic_error If called on moved-from stream
     */
    void synchronize() const;

    /**
     * @brief Check if stream has completed all work
     * 
     * @return true if all queued work is done, false otherwise
     * @throws std::runtime_error If query fails
     * 
     * Safe on moved-from streams (returns true).
     */
    bool is_ready() const noexcept;

    /**
     * @brief Check if this stream is valid and not moved-from
     * 
     * @return true if stream is alive
     */
    bool is_valid() const noexcept;

    /**
     * @brief Check if this stream is in moved-from state
     * 
     * @return true if resources were transferred to another object
     */
    bool is_moved_from() const noexcept;

private:
    void* stream_handle_;
    int device_id_;
    bool is_moved_from_;
};

/**
 * @brief CUDA operation with asynchronous completion tracking
 * 
 * Represents a device-side operation (kernel launch, memory copy, etc.)
 * with move semantics and use-after-move detection.
 */
class CudaOperation {
public:
    /**
     * @brief Operation status enumeration
     */
    enum class Status {
        PENDING,       ///< Operation queued but not started
        RUNNING,       ///< Operation currently executing
        COMPLETED,     ///< Operation finished successfully
        FAILED,        ///< Operation failed
        MOVED_FROM,    ///< Moved-from state (operation transferred to another object)
    };

    /**
     * @brief Create CUDA operation on stream
     * 
     * @param stream CUDA stream to queue operation on
     * @param name Operation name (for logging/debugging)
     * @throws std::invalid_argument If stream is invalid
     */
    CudaOperation(const CudaStream& stream, const std::string& name);

    /**
     * @brief Destructor - ensures operation completion
     * 
     * Safe on moved-from operations (no-op).
     */
    ~CudaOperation() noexcept;

    // Move semantics
    CudaOperation(CudaOperation&& other) noexcept;
    CudaOperation& operator=(CudaOperation&& other) noexcept;

    // No copy
    CudaOperation(const CudaOperation&) = delete;
    CudaOperation& operator=(const CudaOperation&) = delete;

    // --- Operation lifecycle ---

    /**
     * @brief Record event marker in stream
     * 
     * @throws std::runtime_error If event recording fails
     * @throws std::logic_error If called on moved-from operation
     */
    void record_event();

    /**
     * @brief Wait for operation to complete
     * 
     * @param timeout Duration to wait (0 = indefinite)
     * @return true if completed, false if timeout
     * @throws std::runtime_error If synchronization fails
     * @throws std::logic_error If called on moved-from operation
     */
    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * @brief Check current operation status
     * 
     * @return Current Status
     */
    Status get_status() const noexcept;

    /**
     * @brief Mark operation as completed (called internally by CUDA callbacks)
     * 
     * @internal Used by CUDA stream callbacks.
     */
    void mark_completed() noexcept;

    /**
     * @brief Mark operation as failed
     * 
     * @param error_msg Human-readable error message
     */
    void mark_failed(const std::string& error_msg) noexcept;

    // --- Queries ---

    /**
     * @brief Get operation name
     * 
     * @return Name, or empty string if moved-from
     */
    const std::string& get_name() const noexcept;

    /**
     * @brief Get error message if operation failed
     * 
     * @return Error message, or empty string if no error
     */
    const std::string& get_error() const noexcept;

    /**
     * @brief Get device ID where operation runs
     * 
     * @return Device ID, or -1 if moved-from
     */
    int get_device_id() const noexcept;

    /**
     * @brief Check if operation is in moved-from state
     * 
     * @return true if resources were transferred to another object
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

/**
 * @brief Batch of CUDA operations with collective management
 * 
 * Allows submitting multiple operations and waiting for all to complete.
 * Move semantics enable efficient transfer of operation batches.
 */
class CudaOperationBatch {
public:
    /**
     * @brief Create empty operation batch
     * 
     * @param stream CUDA stream for batch operations
     */
    explicit CudaOperationBatch(const CudaStream& stream);

    /**
     * @brief Destructor - waits for batch completion
     */
    ~CudaOperationBatch() noexcept;

    // Move semantics
    CudaOperationBatch(CudaOperationBatch&& other) noexcept;
    CudaOperationBatch& operator=(CudaOperationBatch&& other) noexcept;

    // No copy
    CudaOperationBatch(const CudaOperationBatch&) = delete;
    CudaOperationBatch& operator=(const CudaOperationBatch&) = delete;

    /**
     * @brief Add operation to batch
     * 
     * @param op Operation to add (moved into batch)
     * @throws std::logic_error If called on moved-from batch
     */
    void add_operation(CudaOperation&& op);

    /**
     * @brief Wait for all operations in batch to complete
     * 
     * @param timeout Duration to wait (0 = indefinite)
     * @return true if all completed, false if timeout
     * @throws std::runtime_error If synchronization fails
     * @throws std::logic_error If called on moved-from batch
     */
    bool wait_all(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * @brief Get number of operations in batch
     * 
     * @return Count, or 0 if moved-from
     */
    size_t size() const noexcept;

    /**
     * @brief Check if all operations completed
     * 
     * @return true if all done (or batch empty)
     */
    bool all_completed() const noexcept;

    /**
     * @brief Get count of failed operations
     * 
     * @return Number of operations with errors
     */
    size_t failed_count() const noexcept;

    /**
     * @brief Check if batch is valid (not moved-from)
     * 
     * @return true if batch can accept operations
     */
    bool is_valid() const noexcept;

    /**
     * @brief Check if batch is in moved-from state
     * 
     * @return true if resources were transferred to another object
     */
    bool is_moved_from() const noexcept;

private:
    const CudaStream* stream_;
    std::vector<CudaOperation> operations_;
    bool is_moved_from_;
};

} // namespace gpu
} // namespace themis
