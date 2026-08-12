/**
 * @file wasm_kernel_sandbox.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include "themis/gpu/kernel_validator.h"
#include "themis/gpu/launcher.h"

namespace themis {
namespace gpu {

/**
 * @brief WASM-based GPU kernel sandbox for untrusted third-party kernels.
 *
 * Provides an isolated execution environment for GPU kernel blobs submitted
 * by untrusted third parties.  Two layers of protection are enforced before
 * any kernel is allowed to run:
 *
 * 1. **Whitelist + checksum gate** — delegated to `GPUKernelValidator`.  Only
 *    kernels that have been explicitly registered (kernel_id + expected FNV-1a
 *    checksum) are allowed through.  Unknown IDs and tampered blobs are
 *    rejected immediately.
 *
 * 2. **Sandbox isolation** — when `THEMIS_ENABLE_WASM` is defined the kernel
 *    blob is executed inside a WASM linear-memory sandbox (Wasmtime / WasmEdge)
 *    with a hard memory ceiling (`SandboxConfig::memory_limit_bytes`) and a
 *    wall-clock timeout (`SandboxConfig::max_execution_ms`).  When the WASM
 *    runtime is absent the sandbox falls back to an instrumented CPU execution
 *    path that enforces the same memory bookkeeping and timeout logic.
 *
 * Feature gate
 * ------------
 * The sandbox is gated on `GPUFeatureFlags::Feature::WASM_SANDBOX` which is
 * enabled by default for ENTERPRISE and HYPERSCALER editions only.  Submissions
 * made when the feature is disabled are rejected with
 * `Status::REJECTED_FEATURE_DISABLED`.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class WASMKernelSandbox {
public:
    // -----------------------------------------------------------------------
    // Sandbox configuration
    // -----------------------------------------------------------------------
    struct SandboxConfig {
        /**
         * @brief Maximum linear memory for a single kernel execution in bytes.
         * 0 = no limit enforced in CPU simulation path.
         */
        uint64_t memory_limit_bytes = 64 * 1024 * 1024;  // 64 MiB default

        /**
         * @brief Wall-clock execution timeout per kernel in milliseconds.
         * 0 = no timeout enforced.
         */
        uint32_t max_execution_ms = 5000;  // 5 s default

        /**
         * @brief When true, the sandbox permits host function calls listed in
         * the kernel's capability manifest.  When false all host imports are
         * blocked (strict isolation).
         */
        bool allow_host_calls = false;
    };

    // -----------------------------------------------------------------------
    // Execution status
    // -----------------------------------------------------------------------
    enum class Status {
        OK,                          ///< Kernel executed successfully
        REJECTED_NOT_WHITELISTED,    ///< kernel_id not in the validator whitelist
        REJECTED_CHECKSUM_MISMATCH,  ///< Blob hash does not match registered checksum
        REJECTED_EMPTY_BLOB,         ///< Zero-length blob rejected
        REJECTED_FEATURE_DISABLED,   ///< WASM_SANDBOX feature flag is off
        REJECTED_MEMORY_LIMIT,       ///< Blob size exceeds memory_limit_bytes
        REJECTED_TIMEOUT,            ///< Execution exceeded max_execution_ms
        EXECUTION_ERROR,             ///< Runtime error during sandbox execution
    };

    // -----------------------------------------------------------------------
    // Execution result
    // -----------------------------------------------------------------------
    struct ExecutionResult {
        Status     status    = Status::EXECUTION_ERROR;
        std::string kernel_id;
        std::string message;
        std::chrono::milliseconds elapsed{0};

        bool ok() const noexcept { return status == Status::OK; }
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t   total_submitted         = 0;  ///< Total execute() calls
        size_t   ok_count                = 0;  ///< Successful executions
        size_t   rejected_not_whitelisted = 0; ///< Whitelist rejections
        size_t   rejected_checksum        = 0; ///< Checksum-mismatch rejections
        size_t   rejected_empty           = 0; ///< Empty-blob rejections
        size_t   rejected_feature_disabled = 0;///< Feature-gate rejections
        size_t   rejected_memory_limit    = 0; ///< Memory-limit rejections
        size_t   rejected_timeout         = 0; ///< Timeout rejections
        size_t   execution_errors         = 0; ///< Runtime errors
        uint64_t total_elapsed_ms         = 0; ///< Cumulative execution time
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static WASMKernelSandbox& GetInstance() {
        static WASMKernelSandbox inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct with default sandbox configuration.
     */
    WASMKernelSandbox() = default;

    /**
     * @brief Construct with an explicit sandbox configuration.
     */
    explicit WASMKernelSandbox(SandboxConfig config);

    // Non-copyable.
    WASMKernelSandbox(const WASMKernelSandbox&)            = delete;
    WASMKernelSandbox& operator=(const WASMKernelSandbox&) = delete;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Replace the current sandbox configuration.
     *
     * Thread-safe; effective for all subsequent execute() calls.
     */
    void setConfig(SandboxConfig config);

    /**
     * @brief Return the current sandbox configuration.
     */
    SandboxConfig getConfig() const;

    // -----------------------------------------------------------------------
    // Capability query
    // -----------------------------------------------------------------------

    /**
     * @brief Return true when a WASM runtime is available.
     *
     * Always returns false in the current build (CPU simulation path).
     * Returns true when `THEMIS_ENABLE_WASM` is defined and the runtime
     * initialises successfully.
     */
    bool isWASMSupported() const noexcept;

    // -----------------------------------------------------------------------
    // Execution
    // -----------------------------------------------------------------------

    /**
     * @brief Execute @p blob as a sandboxed GPU kernel.
     *
     * Execution pipeline:
     *  1. Check `WASM_SANDBOX` feature flag → reject if disabled.
     *  2. Reject empty blobs.
     *  3. Reject blobs that exceed `SandboxConfig::memory_limit_bytes`.
     *  4. Validate `kernel_id` + `blob` via `GPUKernelValidator`.
     *  5. Execute in the WASM sandbox (or CPU simulation fallback).
     *  6. Enforce `max_execution_ms` timeout; reject with REJECTED_TIMEOUT
     *     if the deadline is exceeded.
     *
     * @param kernel_id  Kernel identifier (must be registered with
     *                   `GPUKernelValidator`).
     * @param blob       Kernel bytecode / binary to execute.
     * @param backend    Optional execution backend.  When nullptr a CPU no-op
     *                   that always succeeds is used (for testing / CI without
     *                   GPU hardware).
     * @return ExecutionResult describing the outcome.
     */
    ExecutionResult execute(const std::string&          kernel_id,
                            const std::vector<uint8_t>& blob,
                            GPULauncher::BackendFn      backend = nullptr);

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    Stats getStats() const;

    /**
     * @brief Reset all statistics (for testing).
     */
    void resetStats();

private:
    mutable std::mutex mutex_;
    SandboxConfig      config_;

    // Mutable counters (updated under mutex_).
    mutable size_t   total_submitted_          = 0;
    mutable size_t   ok_count_                 = 0;
    mutable size_t   rejected_not_whitelisted_ = 0;
    mutable size_t   rejected_checksum_        = 0;
    mutable size_t   rejected_empty_           = 0;
    mutable size_t   rejected_feature_disabled_ = 0;
    mutable size_t   rejected_memory_limit_    = 0;
    mutable size_t   rejected_timeout_         = 0;
    mutable size_t   execution_errors_         = 0;
    mutable uint64_t total_elapsed_ms_         = 0;

    // Internal helper: execute the kernel payload under CPU simulation.
    // Assumes the mutex is NOT held.
    ExecutionResult runInSandbox(const std::string&          kernel_id,
                                 const std::vector<uint8_t>& blob,
                                 GPULauncher::BackendFn      backend);

    void recordResult(const ExecutionResult& r);
};

/**
 * @brief Human-readable name for a WASMKernelSandbox::Status value.
 */
const char* sandboxStatusName(WASMKernelSandbox::Status s) noexcept;

} // namespace gpu
} // namespace themis
